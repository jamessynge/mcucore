#ifndef AVR_TIMER_COUNTER_ENABLE_MCU_VLOG
#define MCU_DISABLE_VLOG
#endif

#include "platform/avr/timer_counter.h"

#if MCU_HOST_TARGET
#include <ostream>  // pragma: keep standard include
#endif

#include "container/flash_string_table.h"
#include "log/log.h"
#include "print/counting_print.h"
#include "print/has_print_to.h"
#include "print/print_misc.h"
#include "print/print_to_buffer.h"
#include "print/stream_to_print.h"
#include "semistd/type_traits.h"
#include "strings/progmem_string_data.h"

namespace mcucore {

uint16_t ToClockDivisor(ClockPrescaling prescaling) {
  switch (prescaling) {
    case ClockPrescaling::kDivideBy1:
      return 1;
    case ClockPrescaling::kDivideBy8:
      return 8;
    case ClockPrescaling::kDivideBy64:
      return 64;
    case ClockPrescaling::kDivideBy256:
      return 256;
    case ClockPrescaling::kDivideBy1024:
      return 1024;
    default:
      return 0;
  }
}

TC16ClockAndTicks TC16ClockAndTicks::FromSystemClockCycles(
    uint32_t system_clock_cycles) {
  MCU_VLOG(5) << MCU_PSD("FromSystemClockCycles ") << system_clock_cycles;
  MCU_DCHECK_LE(system_clock_cycles, kMaxSystemClockCycles)
      << MCU_PSD("system_clock_cycles: ") << system_clock_cycles
      << MCU_PSD("  kMaxSystemClockCycles: ") << kMaxSystemClockCycles;
  if (system_clock_cycles <= kMaxClockTicks) {
    if (system_clock_cycles == 0) {
      return {.clock_select = ClockPrescaling::kDisabled, .clock_ticks = 0};
    }
    return {.clock_select = ClockPrescaling::kDivideBy1,
            .clock_ticks = static_cast<uint16_t>(system_clock_cycles)};
  } else if (system_clock_cycles <= kMaxClockTicks * 8) {
    return {.clock_select = ClockPrescaling::kDivideBy8,
            .clock_ticks = static_cast<uint16_t>(system_clock_cycles / 8)};
  } else if (system_clock_cycles <= kMaxClockTicks * 64) {
    return {.clock_select = ClockPrescaling::kDivideBy64,
            .clock_ticks = static_cast<uint16_t>(system_clock_cycles / 64)};
  } else if (system_clock_cycles <= kMaxClockTicks * 256) {
    return {.clock_select = ClockPrescaling::kDivideBy256,
            .clock_ticks = static_cast<uint16_t>(system_clock_cycles / 256)};
  } else if (system_clock_cycles <= kMaxClockTicks * 1024) {
    return {.clock_select = ClockPrescaling::kDivideBy1024,
            .clock_ticks = static_cast<uint16_t>(system_clock_cycles / 1024)};
  } else {
    MCU_DCHECK(false) << MCU_PSD("system_clock_cycles: ") << system_clock_cycles
                      << MCU_PSD("  kMaxSystemClockCycles: ")
                      << kMaxSystemClockCycles;
    return {.clock_select = ClockPrescaling::kDisabled, .clock_ticks = 0};
  }
}

TC16ClockAndTicks TC16ClockAndTicks::FromMicroSeconds(uint32_t us) {
  // Convert a duration (us) into the number of system clock cycles for
  // approximately that same duration; it is accurate if an integral number of
  // cycles fits into a microsecond. The following somewhat convoluted
  // expression deals with the precision issues of multiplying and dividing
  // fixed bit size integers. For example (F_CPU / 100,000) is the number of
  // clock cycles in 10 microseconds.
  const uint32_t system_clock_cycles = ((F_CPU / 100000) * us) / 10;
  return FromSystemClockCycles(system_clock_cycles);
}

TC16ClockAndTicks TC16ClockAndTicks::FromNanoSeconds(uint32_t ns) {
  // Working in nanoseconds risks overflow in the 32-bit calculations of the
  // number of clock cycles, therefore I'm dividing this up into separate cases
  // for short periods and long periods. I'm assuming (and verifying) here that
  // kShortPeriodLimit is far longer than a system clock cycle.
  constexpr uint32_t kShortPeriodLimit = (UINT32_MAX - 0) / (F_CPU / 100000);
  static_assert(kShortPeriodLimit / kNanoSecondsPerSystemClockCycle > 100,
                "kShortPeriodLimit is too small");

  constexpr uint32_t half_cycle_ns = 500000000UL / F_CPU;
  if ((ns + half_cycle_ns) >= kShortPeriodLimit) {
    // Round ns to the nearest microsecond.
    return FromMicroSeconds((ns + 500) / 1000);
  }

  // Convert a duration (ns) into the number of system clock cycles that most
  // closely represents the same duration. Some rounding is inevitable because
  // we expect that for microcontrollers the system clock cycle is far long than
  // a nanosecond); we explicitly round ns up by one half of a system clock
  // cycle.
  ns += half_cycle_ns;
  // The following somewhat convoluted expression deals with the precision
  // issues of multiplying and dividing fixed bit size integers. For example
  // (F_CPU / 100,000) is the number of clock cycles in 10 microseconds.
  const uint32_t system_clock_cycles = ((F_CPU / 100000) * ns) / 10000;
  return FromSystemClockCycles(system_clock_cycles);
}

TC16ClockAndTicks TC16ClockAndTicks::FromSeconds(double s) {
  MCU_DCHECK_LE(s, kMaxSeconds);
  if (s > kMaxSeconds) {
    return {.clock_select = ClockPrescaling::kDisabled, .clock_ticks = 0};
  }
  double cycles = s * F_CPU + 0.5;
  MCU_DCHECK_LE(cycles, UINT32_MAX);
  return FromSystemClockCycles(cycles);
}

TC16ClockAndTicks TC16ClockAndTicks::FromIntegerEventsPerSecond(
    uint16_t events) {
  MCU_DCHECK_GT(events, 0);
  if (events > 0) {
    return FromSystemClockCycles(F_CPU / events);
  }
  return {.clock_select = ClockPrescaling::kDisabled, .clock_ticks = 0};
}

TC16ClockAndTicks TC16ClockAndTicks::FromDoubleEventsPerSecond(double events) {
  MCU_DCHECK_GT(events, 0);
  if (events <= 0) {
    return {.clock_select = ClockPrescaling::kDisabled, .clock_ticks = 0};
  }
  return FromSystemClockCycles(F_CPU / events);
}

uint32_t TC16ClockAndTicks::ToSystemClockCycles() const {
  return static_cast<uint32_t>(clock_ticks) * ToClockDivisor(clock_select);
}

double TC16ClockAndTicks::ToSeconds() const {
  return ToSystemClockCycles() / static_cast<double>(F_CPU);
}

size_t TC16ClockAndTicks::printTo(Print& out) const {
  static_assert(mcucore::has_print_to<decltype(*this)>{},
                "mcucore::has_print_to should be true");
  mcucore::CountingPrint counter(out);
  counter << MCU_PSD("{.cs=") << clock_select << MCU_PSD(", .ticks=")
          << clock_ticks << '}';
  return counter.count();
}

////////////////////////////////////////////////////////////////////////////////

namespace {
constexpr uint8_t kKeepAllExceptChannelA =
    static_cast<uint8_t>(~(0b11 << COM1A0));
constexpr uint8_t kKeepAllExceptChannelB =
    static_cast<uint8_t>(~(0b11 << COM1B0));
constexpr uint8_t kKeepAllExceptChannelC =
    static_cast<uint8_t>(~(0b11 << COM1C0));
}  // namespace

void TimerCounter1Initialize16BitFastPwm(ClockPrescaling prescaling) {
  noInterrupts();
  TCCR1A = 1 << WGM11;
  TCCR1B =
      (1 << WGM13) | (1 << WGM12) | static_cast<uint8_t>(prescaling) << CS10;
  ICR1 = UINT16_MAX;
  interrupts();
}

void TimerCounter1SetCompareOutputMode(TimerCounterChannel channel,
                                       FastPwmCompareOutputMode mode) {
  uint8_t keep_mask, set_mask;
  switch (channel) {
    case TimerCounterChannel::A:
      keep_mask = kKeepAllExceptChannelA;
      set_mask = static_cast<uint8_t>(mode) << COM1A0;
      break;
    case TimerCounterChannel::B:
      keep_mask = kKeepAllExceptChannelB;
      set_mask = static_cast<uint8_t>(mode) << COM1B0;
      break;
    case TimerCounterChannel::C:
      keep_mask = kKeepAllExceptChannelC;
      set_mask = static_cast<uint8_t>(mode) << COM1C0;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
  noInterrupts();
  TCCR1A = (TCCR1A & keep_mask) | set_mask;
  interrupts();
}

void TimerCounter1SetOutputCompareRegister(TimerCounterChannel channel,
                                           uint16_t value) {
  switch (channel) {
    case TimerCounterChannel::A:
      OCR1A = value;
      break;
    case TimerCounterChannel::B:
      OCR1B = value;
      break;
    case TimerCounterChannel::C:
      OCR1C = value;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
}

uint16_t TimerCounter1GetOutputCompareRegister(TimerCounterChannel channel) {
  switch (channel) {
    case TimerCounterChannel::A:
      return OCR1A;
    case TimerCounterChannel::B:
      return OCR1B;
    case TimerCounterChannel::C:
      return OCR1C;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
      return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

void TimerCounter3Initialize16BitFastPwm(ClockPrescaling prescaling) {
  noInterrupts();
  TCCR3A = 1 << WGM11;
  TCCR3B =
      (1 << WGM13) | (1 << WGM12) | static_cast<uint8_t>(prescaling) << CS10;
  ICR3 = UINT16_MAX;
  interrupts();
}

void TimerCounter3SetCompareOutputMode(TimerCounterChannel channel,
                                       FastPwmCompareOutputMode mode) {
  uint8_t keep_mask, set_mask;
  switch (channel) {
    case TimerCounterChannel::A:
      keep_mask = kKeepAllExceptChannelA;
      set_mask = static_cast<uint8_t>(mode) << COM3A0;
      break;
    case TimerCounterChannel::B:
      keep_mask = kKeepAllExceptChannelB;
      set_mask = static_cast<uint8_t>(mode) << COM3B0;
      break;
    case TimerCounterChannel::C:
      keep_mask = kKeepAllExceptChannelC;
      set_mask = static_cast<uint8_t>(mode) << COM3C0;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
  noInterrupts();
  TCCR3A = (TCCR3A & keep_mask) | set_mask;
  interrupts();
}

void TimerCounter3SetOutputCompareRegister(TimerCounterChannel channel,
                                           uint16_t value) {
  switch (channel) {
    case TimerCounterChannel::A:
      OCR3A = value;
      break;
    case TimerCounterChannel::B:
      OCR3B = value;
      break;
    case TimerCounterChannel::C:
      OCR3C = value;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
}

uint16_t TimerCounter3GetOutputCompareRegister(TimerCounterChannel channel) {
  switch (channel) {
    case TimerCounterChannel::A:
      return OCR3A;
    case TimerCounterChannel::B:
      return OCR3B;
    case TimerCounterChannel::C:
      return OCR3C;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
      return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

void TimerCounter4Initialize16BitFastPwm(ClockPrescaling prescaling) {
  noInterrupts();
  TCCR4A = 1 << WGM11;
  TCCR4B =
      (1 << WGM13) | (1 << WGM12) | static_cast<uint8_t>(prescaling) << CS10;
  ICR4 = UINT16_MAX;
  interrupts();
}

void TimerCounter4SetCompareOutputMode(TimerCounterChannel channel,
                                       FastPwmCompareOutputMode mode) {
  uint8_t keep_mask, set_mask;
  switch (channel) {
    case TimerCounterChannel::A:
      keep_mask = kKeepAllExceptChannelA;
      set_mask = static_cast<uint8_t>(mode) << COM4A0;
      break;
    case TimerCounterChannel::B:
      keep_mask = kKeepAllExceptChannelB;
      set_mask = static_cast<uint8_t>(mode) << COM4B0;
      break;
    case TimerCounterChannel::C:
      keep_mask = kKeepAllExceptChannelC;
      set_mask = static_cast<uint8_t>(mode) << COM4C0;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
  noInterrupts();
  TCCR4A = (TCCR4A & keep_mask) | set_mask;
  interrupts();
}

void TimerCounter4SetOutputCompareRegister(TimerCounterChannel channel,
                                           uint16_t value) {
  switch (channel) {
    case TimerCounterChannel::A:
      OCR4A = value;
      break;
    case TimerCounterChannel::B:
      OCR4B = value;
      break;
    case TimerCounterChannel::C:
      OCR4C = value;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
}

uint16_t TimerCounter4GetOutputCompareRegister(TimerCounterChannel channel) {
  switch (channel) {
    case TimerCounterChannel::A:
      return OCR4A;
    case TimerCounterChannel::B:
      return OCR4B;
    case TimerCounterChannel::C:
      return OCR4C;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
      return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

void TimerCounter5Initialize16BitFastPwm(ClockPrescaling prescaling) {
  noInterrupts();
  TCCR5A = 1 << WGM11;
  TCCR5B =
      (1 << WGM13) | (1 << WGM12) | static_cast<uint8_t>(prescaling) << CS10;
  ICR5 = UINT16_MAX;
  interrupts();
}

void TimerCounter5SetCompareOutputMode(TimerCounterChannel channel,
                                       FastPwmCompareOutputMode mode) {
  uint8_t keep_mask, set_mask;
  switch (channel) {
    case TimerCounterChannel::A:
      keep_mask = kKeepAllExceptChannelA;
      set_mask = static_cast<uint8_t>(mode) << COM5A0;
      break;
    case TimerCounterChannel::B:
      keep_mask = kKeepAllExceptChannelB;
      set_mask = static_cast<uint8_t>(mode) << COM5B0;
      break;
    case TimerCounterChannel::C:
      keep_mask = kKeepAllExceptChannelC;
      set_mask = static_cast<uint8_t>(mode) << COM5C0;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
  noInterrupts();
  TCCR5A = (TCCR5A & keep_mask) | set_mask;
  interrupts();
}

void TimerCounter5SetOutputCompareRegister(TimerCounterChannel channel,
                                           uint16_t value) {
  switch (channel) {
    case TimerCounterChannel::A:
      OCR5A = value;
      break;
    case TimerCounterChannel::B:
      OCR5B = value;
      break;
    case TimerCounterChannel::C:
      OCR5C = value;
      break;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
  }
}

uint16_t TimerCounter5GetOutputCompareRegister(TimerCounterChannel channel) {
  switch (channel) {
    case TimerCounterChannel::A:
      return OCR5A;
    case TimerCounterChannel::B:
      return OCR5B;
    case TimerCounterChannel::C:
      return OCR5C;
    default:
      MCU_DCHECK(false) << MCU_PSD("Unknown channel ") << channel;
      return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

EnableableByPin::EnableableByPin(uint8_t enabled_pin)
    : enabled_pin_(enabled_pin) {}

EnableableByPin::EnableableByPin() : enabled_pin_(kNoEnabledPin) {}

bool EnableableByPin::IsEnabled() const {
  if (enabled_pin_ == kNoEnabledPin) {
    return true;
  }
  pinMode(enabled_pin_, INPUT_PULLUP);
  bool result = digitalRead(enabled_pin_) == LOW;
  pinMode(enabled_pin_, INPUT);  // Avoid supplying current unncessarily.
  return result;
}

int EnableableByPin::ReadPin() const {
  if (enabled_pin_ == kNoEnabledPin) {
    return -1;
  }
  pinMode(enabled_pin_, INPUT_PULLUP);
  delayMicroseconds(50);
  int result = digitalRead(enabled_pin_);
  pinMode(enabled_pin_, INPUT);  // Avoid supplying current unncessarily.
  return result;
}

////////////////////////////////////////////////////////////////////////////////

TimerCounter1Pwm16Output::TimerCounter1Pwm16Output(TimerCounterChannel channel,
                                                   uint8_t enabled_pin)
    : EnableableByPin(enabled_pin), channel_(channel) {}
TimerCounter1Pwm16Output::TimerCounter1Pwm16Output(TimerCounterChannel channel)
    : channel_(channel) {}

void TimerCounter1Pwm16Output::set_pulse_count(uint16_t value) {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 1") << MCU_PSD(" is not enabled");
  if (value == 0) {
    TimerCounter1SetCompareOutputMode(channel_,
                                      FastPwmCompareOutputMode::kDisabled);
  } else {
    TimerCounter1SetCompareOutputMode(
        channel_, FastPwmCompareOutputMode::kNonInvertingMode);
    TimerCounter1SetOutputCompareRegister(channel_, value);
  }
}

uint16_t TimerCounter1Pwm16Output::get_pulse_count() const {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 1") << MCU_PSD(" is not enabled");
  return TimerCounter1GetOutputCompareRegister(channel_);
}

////////////////////////////////////////////////////////////////////////////////

TimerCounter3Pwm16Output::TimerCounter3Pwm16Output(TimerCounterChannel channel,
                                                   uint8_t enabled_pin)
    : EnableableByPin(enabled_pin), channel_(channel) {}
TimerCounter3Pwm16Output::TimerCounter3Pwm16Output(TimerCounterChannel channel)
    : channel_(channel) {}

void TimerCounter3Pwm16Output::set_pulse_count(uint16_t value) {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 3") << MCU_PSD(" is not enabled");
  if (value == 0) {
    TimerCounter3SetCompareOutputMode(channel_,
                                      FastPwmCompareOutputMode::kDisabled);
  } else {
    TimerCounter3SetCompareOutputMode(
        channel_, FastPwmCompareOutputMode::kNonInvertingMode);
    TimerCounter3SetOutputCompareRegister(channel_, value);
  }
}

uint16_t TimerCounter3Pwm16Output::get_pulse_count() const {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 3") << MCU_PSD(" is not enabled");
  return TimerCounter3GetOutputCompareRegister(channel_);
}

////////////////////////////////////////////////////////////////////////////////

TimerCounter4Pwm16Output::TimerCounter4Pwm16Output(TimerCounterChannel channel,
                                                   uint8_t enabled_pin)
    : EnableableByPin(enabled_pin), channel_(channel) {}
TimerCounter4Pwm16Output::TimerCounter4Pwm16Output(TimerCounterChannel channel)
    : channel_(channel) {}

void TimerCounter4Pwm16Output::set_pulse_count(uint16_t value) {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 4") << MCU_PSD(" is not enabled");
  if (value == 0) {
    TimerCounter4SetCompareOutputMode(channel_,
                                      FastPwmCompareOutputMode::kDisabled);
  } else {
    TimerCounter4SetCompareOutputMode(
        channel_, FastPwmCompareOutputMode::kNonInvertingMode);
    TimerCounter4SetOutputCompareRegister(channel_, value);
  }
}

uint16_t TimerCounter4Pwm16Output::get_pulse_count() const {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 4") << MCU_PSD(" is not enabled");
  return TimerCounter4GetOutputCompareRegister(channel_);
}

////////////////////////////////////////////////////////////////////////////////

TimerCounter5Pwm16Output::TimerCounter5Pwm16Output(TimerCounterChannel channel,
                                                   uint8_t enabled_pin)
    : EnableableByPin(enabled_pin), channel_(channel) {}
TimerCounter5Pwm16Output::TimerCounter5Pwm16Output(TimerCounterChannel channel)
    : channel_(channel) {}

void TimerCounter5Pwm16Output::set_pulse_count(uint16_t value) {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 5") << MCU_PSD(" is not enabled");
  if (value == 0) {
    TimerCounter5SetCompareOutputMode(channel_,
                                      FastPwmCompareOutputMode::kDisabled);
  } else {
    TimerCounter5SetCompareOutputMode(
        channel_, FastPwmCompareOutputMode::kNonInvertingMode);
    TimerCounter5SetOutputCompareRegister(channel_, value);
  }
}

uint16_t TimerCounter5Pwm16Output::get_pulse_count() const {
  MCU_VLOG_IF(1, !IsEnabled())
      << MCU_PSD("T/C 5") << MCU_PSD(" is not enabled");
  return TimerCounter5GetOutputCompareRegister(channel_);
}

}  // namespace mcucore

// BEGIN_SOURCE_GENERATED_BY_MAKE_ENUM_TO_STRING

namespace mcucore {
namespace {

MCU_MAYBE_UNUSED_FUNCTION inline const __FlashStringHelper*
_ToFlashStringHelperViaSwitch(ClockPrescaling v) {
  switch (v) {
    case ClockPrescaling::kDisabled:
      return MCU_FLASHSTR("Disabled");
    case ClockPrescaling::kDivideBy1:
      return MCU_FLASHSTR("DivideBy1");
    case ClockPrescaling::kDivideBy8:
      return MCU_FLASHSTR("DivideBy8");
    case ClockPrescaling::kDivideBy64:
      return MCU_FLASHSTR("DivideBy64");
    case ClockPrescaling::kDivideBy256:
      return MCU_FLASHSTR("DivideBy256");
    case ClockPrescaling::kDivideBy1024:
      return MCU_FLASHSTR("DivideBy1024");
  }
  return nullptr;
}

}  // namespace

const __FlashStringHelper* ToFlashStringHelper(ClockPrescaling v) {
#ifdef TO_FLASH_STRING_HELPER_PREFER_SWITCH
  return _ToFlashStringHelperViaSwitch(v);
#else  // not TO_FLASH_STRING_HELPER_PREFER_SWITCH
#ifdef TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS
  if (v == ClockPrescaling::kDisabled) {
    return MCU_FLASHSTR("Disabled");
  }
  if (v == ClockPrescaling::kDivideBy1) {
    return MCU_FLASHSTR("DivideBy1");
  }
  if (v == ClockPrescaling::kDivideBy8) {
    return MCU_FLASHSTR("DivideBy8");
  }
  if (v == ClockPrescaling::kDivideBy64) {
    return MCU_FLASHSTR("DivideBy64");
  }
  if (v == ClockPrescaling::kDivideBy256) {
    return MCU_FLASHSTR("DivideBy256");
  }
  if (v == ClockPrescaling::kDivideBy1024) {
    return MCU_FLASHSTR("DivideBy1024");
  }
  return nullptr;
#else   // not TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS
  // Protection against enumerator definitions changing:
  static_assert(ClockPrescaling::kDisabled == static_cast<ClockPrescaling>(0));
  static_assert(ClockPrescaling::kDivideBy1 == static_cast<ClockPrescaling>(1));
  static_assert(ClockPrescaling::kDivideBy8 == static_cast<ClockPrescaling>(2));
  static_assert(ClockPrescaling::kDivideBy64 ==
                static_cast<ClockPrescaling>(3));
  static_assert(ClockPrescaling::kDivideBy256 ==
                static_cast<ClockPrescaling>(4));
  static_assert(ClockPrescaling::kDivideBy1024 ==
                static_cast<ClockPrescaling>(5));
  static MCU_FLASH_STRING_TABLE(  // Force new line.
      flash_string_table,
      MCU_PSD("Disabled"),      // 0: kDisabled
      MCU_PSD("DivideBy1"),     // 1: kDivideBy1
      MCU_PSD("DivideBy8"),     // 2: kDivideBy8
      MCU_PSD("DivideBy64"),    // 3: kDivideBy64
      MCU_PSD("DivideBy256"),   // 4: kDivideBy256
      MCU_PSD("DivideBy1024"),  // 5: kDivideBy1024
  );
  return mcucore::LookupFlashStringForDenseEnum<uint8_t>(flash_string_table, v);
#endif  // TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS
#endif  // TO_FLASH_STRING_HELPER_PREFER_SWITCH
}

namespace {

MCU_MAYBE_UNUSED_FUNCTION inline const __FlashStringHelper*
_ToFlashStringHelperViaSwitch(FastPwmCompareOutputMode v) {
  switch (v) {
    case FastPwmCompareOutputMode::kDisabled:
      return MCU_FLASHSTR("Disabled");
    case FastPwmCompareOutputMode::kNonInvertingMode:
      return MCU_FLASHSTR("NonInvertingMode");
    case FastPwmCompareOutputMode::kInvertingMode:
      return MCU_FLASHSTR("InvertingMode");
  }
  return nullptr;
}

}  // namespace

const __FlashStringHelper* ToFlashStringHelper(FastPwmCompareOutputMode v) {
#ifdef TO_FLASH_STRING_HELPER_PREFER_SWITCH
  return _ToFlashStringHelperViaSwitch(v);
#else   // not TO_FLASH_STRING_HELPER_PREFER_SWITCH
  if (v == FastPwmCompareOutputMode::kDisabled) {
    return MCU_FLASHSTR("Disabled");
  }
  if (v == FastPwmCompareOutputMode::kNonInvertingMode) {
    return MCU_FLASHSTR("NonInvertingMode");
  }
  if (v == FastPwmCompareOutputMode::kInvertingMode) {
    return MCU_FLASHSTR("InvertingMode");
  }
  return nullptr;
#endif  // TO_FLASH_STRING_HELPER_PREFER_SWITCH
}

namespace {

MCU_MAYBE_UNUSED_FUNCTION inline const __FlashStringHelper*
_ToFlashStringHelperViaSwitch(TimerCounterChannel v) {
  switch (v) {
    case TimerCounterChannel::A:
      return MCU_FLASHSTR("A");
    case TimerCounterChannel::B:
      return MCU_FLASHSTR("B");
    case TimerCounterChannel::C:
      return MCU_FLASHSTR("C");
  }
  return nullptr;
}

}  // namespace

const __FlashStringHelper* ToFlashStringHelper(TimerCounterChannel v) {
#ifdef TO_FLASH_STRING_HELPER_PREFER_SWITCH
  return _ToFlashStringHelperViaSwitch(v);
#else  // not TO_FLASH_STRING_HELPER_PREFER_SWITCH
#ifdef TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS
  if (v == TimerCounterChannel::A) {
    return MCU_FLASHSTR("A");
  }
  if (v == TimerCounterChannel::B) {
    return MCU_FLASHSTR("B");
  }
  if (v == TimerCounterChannel::C) {
    return MCU_FLASHSTR("C");
  }
  return nullptr;
#else   // not TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS
  // Protection against enumerator definitions changing:
  static_assert(TimerCounterChannel::A == static_cast<TimerCounterChannel>(0));
  static_assert(TimerCounterChannel::B == static_cast<TimerCounterChannel>(1));
  static_assert(TimerCounterChannel::C == static_cast<TimerCounterChannel>(2));
  static MCU_FLASH_STRING_TABLE(  // Force new line.
      flash_string_table,
      MCU_PSD("A"),  // 0: A
      MCU_PSD("B"),  // 1: B
      MCU_PSD("C"),  // 2: C
  );
  return mcucore::LookupFlashStringForDenseEnum<uint8_t>(flash_string_table, v);
#endif  // TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS
#endif  // TO_FLASH_STRING_HELPER_PREFER_SWITCH
}

size_t PrintValueTo(ClockPrescaling v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("ClockPrescaling"),
                                          static_cast<uint32_t>(v), out);
}

size_t PrintValueTo(FastPwmCompareOutputMode v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(
      MCU_FLASHSTR("FastPwmCompareOutputMode"), static_cast<uint32_t>(v), out);
}

size_t PrintValueTo(TimerCounterChannel v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("TimerCounterChannel"),
                                          static_cast<uint32_t>(v), out);
}

#if MCU_HOST_TARGET
// Support for debug logging of enums.

std::ostream& operator<<(std::ostream& os, ClockPrescaling v) {
  char buffer[256];
  mcucore::PrintToBuffer print(buffer);
  PrintValueTo(v, print);
  return os << std::string_view(buffer, print.data_size());
}

std::ostream& operator<<(std::ostream& os, FastPwmCompareOutputMode v) {
  char buffer[256];
  mcucore::PrintToBuffer print(buffer);
  PrintValueTo(v, print);
  return os << std::string_view(buffer, print.data_size());
}

std::ostream& operator<<(std::ostream& os, TimerCounterChannel v) {
  char buffer[256];
  mcucore::PrintToBuffer print(buffer);
  PrintValueTo(v, print);
  return os << std::string_view(buffer, print.data_size());
}

#endif  // MCU_HOST_TARGET

}  // namespace mcucore

// END_SOURCE_GENERATED_BY_MAKE_ENUM_TO_STRING
