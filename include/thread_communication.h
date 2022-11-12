#pragma once

#include "generator.h"
#include "oscillator.h"
#include "util.h"

#include <farbot/AsyncCaller.hpp>
#include <farbot/fifo.hpp>
#include <farbot/RealtimeObject.hpp>

#include <queue>
#include <unordered_map>

// This file defines communication mechanisms between the realtime and
// non-realtime threads. This program is designed such that there are two
// threads running: the UI (main) thread, running with lower priority, captures
// user input and sends requests on a lock-free request queue to the realtime
// thread. The realtime thread processes those requests, making modifications
// to the oscillator. The realtime thread then sends a response on a different
// lock-free queue back to the UI thread indicating the results of the request.
// This setup ensures that there are no data races on the generator settings,
// and allows for smoothly triggering transitions to different oscillator states.
// If the realtime thread needs potentially expensive code executed (such as
// system calls), it can defer that code to the UI thread via another queue.

struct UIOscillatorView;

using RequestId = uint32_t;

// Define events to be passed between threads on lock-free queues.
namespace Events
{
    namespace ModifyGenerator
    {
        // Requests
        enum class Action : uint8_t
        {
            AddOscillator,
            RemoveOscillator,
            ActivateOscillator,
            DeactivateOscillator,
            SetOscillatorFrequency,
            SetOscillatorVolume,
            SetOscillatorPan,
            SetOscillatorType
        };

        struct Request
        {
            RequestId id{ 0 };
            Action action{};

            virtual ~Request() = default; // avoid memory leaks when deleting
        };

        struct AddOscillatorRequest : Request
        {
            OscillatorSettings settings;
        };

        struct RemoveOscillatorRequest : Request
        {
            OscillatorId idToRemove{};
        };

        struct ModifyOscillatorRequest : Request
        {
            OscillatorId idToModify{};
        };

        struct ActivateOscillatorRequest     : ModifyOscillatorRequest { volume_t       volume{}; };
        struct DeactivateOscillatorRequest   : ModifyOscillatorRequest { };
        struct SetOscillatorFrequencyRequest : ModifyOscillatorRequest { frequency_t    newFrequency{}; };
        struct SetOscillatorVolumeRequest    : ModifyOscillatorRequest { volume_t       newVolume{}; };
        struct SetOscillatorPanRequest       : ModifyOscillatorRequest { pan_t          newPan{}; };
        struct SetOscillatorTypeRequest      : ModifyOscillatorRequest { OscillatorType newType{}; };

        // Responses
        enum class Result : uint8_t
        {
            AddOscillatorSucceeded,
            AddOscillatorFailed,
            RemoveOscillatorSucceeded,
            RemoveOscillatorFailed,
            ActivateOscillatorSucceeded,
            ActivateOscillatorFailed,
            DeactivateOscillatorSucceeded,
            DeactivateOscillatorFailed,
            SetOscillatorFrequencySucceeded,
            SetOscillatorFrequencyFailed,
            SetOscillatorVolumeSucceeded,
            SetOscillatorVolumeFailed,
            SetOscillatorPanSucceeded,
            SetOscillatorPanFailed,
            SetOscillatorTypeSucceeded,
            SetOscillatorTypeFailed
        };

        // Not inheritance to avoid allocating on the realtime thread.
        struct Response
        {
            RequestId requestId{ 0 }; // the request to which this response corresponds
            // TODO: put the response type here. make result either success or failure. remove most Result enum vals
            Result result{};
            std::optional<OscillatorId> oscillatorId;             // for add/remove/modify oscillator requests
            std::optional<OscillatorSettings> oscillatorSettings; // for add oscillator requests

            // For oscillator modifications
            std::optional<frequency_t> frequency;
            std::optional<volume_t> volume;
            std::optional<pan_t> pan;
            std::optional<OscillatorType> type;
        };
    }
}

struct ThreadCommunication
{
    using RequestQueueType =
        farbot::fifo<std::unique_ptr<const Events::ModifyGenerator::Request>,
        farbot::fifo_options::concurrency::single, // consumer
        farbot::fifo_options::concurrency::single, // producer
        farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // consumer
        farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // producer
        2>; // max threads

    using ResponseQueueType =
        farbot::fifo<Events::ModifyGenerator::Response,
        farbot::fifo_options::concurrency::single, // consumer
        farbot::fifo_options::concurrency::single, // producer
        farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // consumer
        farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // producer
        2>; // max threads

    using AsyncCallerType = farbot::AsyncCaller<farbot::fifo_options::concurrency::single>;

    // Get a reference to the generator settings event queue, used for
    // passing messages from the non-realtime thread to the realtime thread.
    static RequestQueueType& getModifyGeneratorRequestQueue();

    // Get a reference to the oscillator modification result event queue, used for
    // passing messages about the status of an oscillator modification request from
    // the realtime thread to the non-realtime thread.
    static ResponseQueueType& getModifyGeneratorResponseQueue();

    // Get a reference to the AsyncCaller, a mechanism for dispatching lambdas to the
    // non-realtime thread without waiting or blocking. Useful for deferring stuff.
    static AsyncCallerType& getRealtimeAsyncCaller();

    // Call on the realtime thread to defer execution to the non-realtime thread.
    // Useful for deleting memory or any other syscall-inducing functionality.
    static bool deferToNonRealtimeThread(std::function<void()>&& fn);

    // Call on the non-realtime thread to run deferred code.
    static bool processDeferredActions();
};

struct GeneratorAccess
{
    // Get a reference to the generator.
    static Generator<>& getInstance();
};

// The functions in this namespace help the UI thread push events to
// the realtime thread via the modify generator request queue.
namespace EventBuilder
{
    bool PushAddOscillatorEvent(RequestId requestId, OscillatorSettings settings);
    bool PushRemoveOscillatorEvent(RequestId requestId, OscillatorId idToRemove);
    bool PushActivateOscillatorEvent(RequestId requestId, OscillatorId idToModify, volume_t volume);
    bool PushDeactivateOscillatorEvent(RequestId requestId, OscillatorId idToModify);
    bool PushSetOscillatorFrequencyEvent(RequestId requestId, OscillatorId idToModify, frequency_t frequency);
    bool PushSetOscillatorVolumeEvent(RequestId requestId, OscillatorId idToModify, volume_t volume);
    bool PushSetOscillatorPanEvent(RequestId requestId, OscillatorId idToModify, pan_t pan);
    bool PushSetOscillatorTypeEvent(RequestId requestId, OscillatorId idToModify, OscillatorType type);
}

// TODO: add request type to params. add bool success to params. add request type to response. simplify ::result enum
//Events::ModifyGenerator::Response CreateResponse(RequestId requestId, OscillatorId oscillatorId)
//{
//    Events::ModifyGenerator::Response response;
//    response.requestId = requestId;
//    response.oscillatorId = oscillatorId;
//    return response;
//}

bool DispatchModifyGeneratorRequest(const Events::ModifyGenerator::Request& request);

// Read from the generator request queue; handle all requests.
// Respond to each request to alert the UI thread what happened.
// This function is meant to be called by the realtime thread;
// it is in charge of honoring requests that it modify its settings.
void ProcessModifyGeneratorRequests();

std::queue<RequestId>& GetRequestIds();

UIOscillatorView& GetUIOscillatorView();
