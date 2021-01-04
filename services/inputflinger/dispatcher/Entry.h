/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _UI_INPUT_INPUTDISPATCHER_ENTRY_H
#define _UI_INPUT_INPUTDISPATCHER_ENTRY_H

#include "InjectionState.h"
#include "InputTarget.h"

#include <input/Input.h>
#include <input/InputApplication.h>
#include <stdint.h>
#include <utils/Timers.h>
#include <functional>
#include <string>

namespace android::inputdispatcher {

struct EventEntry {
    enum class Type {
        CONFIGURATION_CHANGED,
        DEVICE_RESET,
        FOCUS,
        KEY,
        MOTION,
    };

    static const char* typeToString(Type type) {
        switch (type) {
            case Type::CONFIGURATION_CHANGED:
                return "CONFIGURATION_CHANGED";
            case Type::DEVICE_RESET:
                return "DEVICE_RESET";
            case Type::FOCUS:
                return "FOCUS";
            case Type::KEY:
                return "KEY";
            case Type::MOTION:
                return "MOTION";
        }
    }

    int32_t id;
    Type type;
    nsecs_t eventTime;
    uint32_t policyFlags;
    InjectionState* injectionState;

    bool dispatchInProgress; // initially false, set to true while dispatching

    /**
     * Injected keys are events from an external (probably untrusted) application
     * and are not related to real hardware state. They come in via
     * InputDispatcher::injectInputEvent, which sets policy flag POLICY_FLAG_INJECTED.
     */
    inline bool isInjected() const { return injectionState != nullptr; }

    /**
     * Synthesized events are either injected events, or events that come
     * from real hardware, but aren't directly attributable to a specific hardware event.
     * Key repeat is a synthesized event, because it is related to an actual hardware state
     * (a key is currently pressed), but the repeat itself is generated by the framework.
     */
    inline bool isSynthesized() const {
        return isInjected() || IdGenerator::getSource(id) != IdGenerator::Source::INPUT_READER;
    }

    virtual std::string getDescription() const = 0;

    EventEntry(int32_t id, Type type, nsecs_t eventTime, uint32_t policyFlags);
    virtual ~EventEntry();

protected:
    void releaseInjectionState();
};

struct ConfigurationChangedEntry : EventEntry {
    explicit ConfigurationChangedEntry(int32_t id, nsecs_t eventTime);
    std::string getDescription() const override;

    virtual ~ConfigurationChangedEntry();
};

struct DeviceResetEntry : EventEntry {
    int32_t deviceId;

    DeviceResetEntry(int32_t id, nsecs_t eventTime, int32_t deviceId);
    std::string getDescription() const override;

    virtual ~DeviceResetEntry();
};

struct FocusEntry : EventEntry {
    sp<IBinder> connectionToken;
    bool hasFocus;
    std::string_view reason;

    FocusEntry(int32_t id, nsecs_t eventTime, sp<IBinder> connectionToken, bool hasFocus,
               std::string_view reason);
    std::string getDescription() const override;

    virtual ~FocusEntry();
};

struct KeyEntry : EventEntry {
    int32_t deviceId;
    uint32_t source;
    int32_t displayId;
    int32_t action;
    int32_t flags;
    int32_t keyCode;
    int32_t scanCode;
    int32_t metaState;
    int32_t repeatCount;
    nsecs_t downTime;

    bool syntheticRepeat; // set to true for synthetic key repeats

    enum InterceptKeyResult {
        INTERCEPT_KEY_RESULT_UNKNOWN,
        INTERCEPT_KEY_RESULT_SKIP,
        INTERCEPT_KEY_RESULT_CONTINUE,
        INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER,
    };
    InterceptKeyResult interceptKeyResult; // set based on the interception result
    nsecs_t interceptKeyWakeupTime;        // used with INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER

    KeyEntry(int32_t id, nsecs_t eventTime, int32_t deviceId, uint32_t source, int32_t displayId,
             uint32_t policyFlags, int32_t action, int32_t flags, int32_t keyCode, int32_t scanCode,
             int32_t metaState, int32_t repeatCount, nsecs_t downTime);
    std::string getDescription() const override;
    void recycle();

    virtual ~KeyEntry();
};

struct MotionEntry : EventEntry {
    int32_t deviceId;
    uint32_t source;
    int32_t displayId;
    int32_t action;
    int32_t actionButton;
    int32_t flags;
    int32_t metaState;
    int32_t buttonState;
    MotionClassification classification;
    int32_t edgeFlags;
    float xPrecision;
    float yPrecision;
    float xCursorPosition;
    float yCursorPosition;
    nsecs_t downTime;
    uint32_t pointerCount;
    PointerProperties pointerProperties[MAX_POINTERS];
    PointerCoords pointerCoords[MAX_POINTERS];

    MotionEntry(int32_t id, nsecs_t eventTime, int32_t deviceId, uint32_t source, int32_t displayId,
                uint32_t policyFlags, int32_t action, int32_t actionButton, int32_t flags,
                int32_t metaState, int32_t buttonState, MotionClassification classification,
                int32_t edgeFlags, float xPrecision, float yPrecision, float xCursorPosition,
                float yCursorPosition, nsecs_t downTime, uint32_t pointerCount,
                const PointerProperties* pointerProperties, const PointerCoords* pointerCoords,
                float xOffset, float yOffset);
    std::string getDescription() const override;

    virtual ~MotionEntry();
};

// Tracks the progress of dispatching a particular event to a particular connection.
struct DispatchEntry {
    const uint32_t seq; // unique sequence number, never 0

    std::shared_ptr<EventEntry> eventEntry; // the event to dispatch
    int32_t targetFlags;
    ui::Transform transform;
    float globalScaleFactor;
    // Both deliveryTime and timeoutTime are only populated when the entry is sent to the app,
    // and will be undefined before that.
    nsecs_t deliveryTime; // time when the event was actually delivered
    // An ANR will be triggered if a response for this entry is not received by timeoutTime
    nsecs_t timeoutTime;

    // Set to the resolved ID, action and flags when the event is enqueued.
    int32_t resolvedEventId;
    int32_t resolvedAction;
    int32_t resolvedFlags;

    DispatchEntry(std::shared_ptr<EventEntry> eventEntry, int32_t targetFlags,
                  ui::Transform transform, float globalScaleFactor);

    inline bool hasForegroundTarget() const { return targetFlags & InputTarget::FLAG_FOREGROUND; }

    inline bool isSplit() const { return targetFlags & InputTarget::FLAG_SPLIT; }

private:
    static volatile int32_t sNextSeqAtomic;

    static uint32_t nextSeq();
};

VerifiedKeyEvent verifiedKeyEventFromKeyEntry(const KeyEntry& entry);
VerifiedMotionEvent verifiedMotionEventFromMotionEntry(const MotionEntry& entry);

class InputDispatcher;
// A command entry captures state and behavior for an action to be performed in the
// dispatch loop after the initial processing has taken place.  It is essentially
// a kind of continuation used to postpone sensitive policy interactions to a point
// in the dispatch loop where it is safe to release the lock (generally after finishing
// the critical parts of the dispatch cycle).
//
// The special thing about commands is that they can voluntarily release and reacquire
// the dispatcher lock at will.  Initially when the command starts running, the
// dispatcher lock is held.  However, if the command needs to call into the policy to
// do some work, it can release the lock, do the work, then reacquire the lock again
// before returning.
//
// This mechanism is a bit clunky but it helps to preserve the invariant that the dispatch
// never calls into the policy while holding its lock.
//
// Commands are implicitly 'LockedInterruptible'.
struct CommandEntry;
typedef std::function<void(InputDispatcher&, CommandEntry*)> Command;

class Connection;
struct CommandEntry {
    explicit CommandEntry(Command command);
    ~CommandEntry();

    Command command;

    // parameters for the command (usage varies by command)
    sp<Connection> connection;
    nsecs_t eventTime;
    std::shared_ptr<KeyEntry> keyEntry;
    std::shared_ptr<InputApplicationHandle> inputApplicationHandle;
    std::string reason;
    int32_t userActivityEventType;
    uint32_t seq;
    bool handled;
    sp<IBinder> connectionToken;
    sp<IBinder> oldToken;
    sp<IBinder> newToken;
    std::string obscuringPackage;
};

} // namespace android::inputdispatcher

#endif // _UI_INPUT_INPUTDISPATCHER_ENTRY_H
