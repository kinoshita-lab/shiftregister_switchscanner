/**
 * @file	shiftregister_switchscanner.hpp
 * @brief	Header only shift register switch scanner library
 * @author Kazuki Saita <saita@kinoshita-lab.com>
 * Copyright (c) 2025 Kinoshita Laboratory All rights reserved.
 */
#pragma once
#ifndef SHIFTREGISTER_SWITCHSCANNER_HPP
#define SHIFTREGISTER_SWITCHSCANNER_HPP

#include <Arduino.h>
#include <functional>
#include <array>

namespace kinoshita_lab
{
template <uint8_t num_switches, uint8_t scan_period, uint8_t pin_npl, uint8_t pin_clock, uint8_t pin_output>
class ShiftregisterSwitchScanner
{
private:
    enum InternalState
    {
        kInit,
        kLoadStart,
        kReadEachBits,
        kWaitNext,
        kNumNormalStates,
        kUnknownState = 0xff,
    };

public:
    using SwitchChangeHandler = std::function<void(uint32_t switch_index, const int off_on)>;

    ShiftregisterSwitchScanner()
    {
        // initialize switch status
        for (auto i = 0u; i < num_switches; ++i) {
            scan_buffers_[i]        = 1;
            former_scan_buffers_[i] = 1;
            switch_status_[i]       = 1;
        }
        pinMode(pin_npl, OUTPUT);
        pinMode(pin_clock, OUTPUT);
        pinMode(pin_output, INPUT_PULLUP);

        setState(kInit);
    }
    
    virtual ~ShiftregisterSwitchScanner() = default;

    void onChange(SwitchChangeHandler handler)
    {
        handler_ = handler;
    }


    void update()
    {
        switch (status_) {
        case kInit:
            setState(kLoadStart);
            break;
        case kLoadStart:
            setState(kReadEachBits);
            break;
        case kReadEachBits: {
            for (auto ic_index = 0u; ic_index < num_switches / 8; ic_index++) {
                constexpr auto num_bits = 8;
                for (auto i = 0u; i < num_bits; ++i) {
                    digitalWrite(pin_clock, LOW);
                    const auto read_data                                    = digitalRead(pin_output);
                    scan_buffers_[(ic_index * num_bits) + num_bits - i - 1] = read_data;
                    digitalWrite(pin_clock, HIGH);
                }
            }
            updateSwitchStatus();
            setState(kWaitNext);
        } break;
        case kWaitNext: {
            const auto current = millis();
            const auto delta   = current - wait_start_millis_;
            if (delta >= scan_period) {
                setState(kLoadStart);
            }
        } break;
        default:
            break;
        }
    }

    void forceScan()
    {
        // scan 2 times just in case for robustness
        for (auto i = 0; i < 2; ++i) {
            for (auto i = 0; i < kNumNormalStates; ++i) {
                update();
                delay(scan_period);
            }
        }
    }

    bool switchIsOn(const uint32_t switch_index) const
    {
        if (switch_index >= num_switches) {
            return false;
        }

        return switch_status_[switch_index] == 0;
    }

protected:
    uint8_t status_ = kUnknownState;

    void setState(const int status)
    {
        status_ = status;
        switch (status_) {
        case kInit:
            digitalWrite(pin_npl, HIGH);
            digitalWrite(pin_clock, LOW);
            break;
        case kLoadStart:
            digitalWrite(pin_npl, LOW);
            digitalWrite(pin_clock, LOW);
            break;
        case kReadEachBits:
            digitalWrite(pin_clock, LOW);
            digitalWrite(pin_npl, HIGH);
            break;
        case kWaitNext:
            wait_start_millis_ = millis();
            digitalWrite(pin_npl, HIGH);
            digitalWrite(pin_clock, LOW);
            break;
        default:
            break;
        }
    }

    void updateSwitchStatus()
    {
        for (auto i = 0u; i < num_switches; ++i) {
            if (scan_buffers_[i] == former_scan_buffers_[i]) {
                const auto new_status = scan_buffers_[i];
                auto current_status   = switch_status_[i];
                if (current_status != new_status) {
                    switch_status_[i] = (new_status);

                    const auto notification_status = !switch_status_[i]; // NOTE: inverted!! off = HIGH, on = LOW
                    if (handler_) {
                        handler_(i, notification_status);
                    }
                }
            }
            former_scan_buffers_[i] = scan_buffers_[i];
        }
    }

    SwitchChangeHandler handler_ = nullptr;

    std::array<uint8_t, num_switches> scan_buffers_        = {0};
    std::array<uint8_t, num_switches> former_scan_buffers_ = {0};
    std::array<uint8_t, num_switches> switch_status_       = {0};
    uint32_t wait_start_millis_                            = 0;

private:
    ShiftregisterSwitchScanner(const ShiftregisterSwitchScanner&) {}
};
}

#endif // SHIFTREGISTER_SWITCHSCANNER_HPP
