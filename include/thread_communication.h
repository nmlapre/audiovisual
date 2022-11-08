#pragma once

#include "generator.h"
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

namespace ThreadCommunication
{
    using namespace farbot;

    // Get a reference to the generator settings event queue, used for
    // passing messages from the non-realtime thread to the realtime thread.
    static __forceinline auto& getModifyGeneratorRequestQueue()
    {
        static const int REQUEST_QUEUE_SIZE = 32;
        static_assert(isPowerOf2(REQUEST_QUEUE_SIZE));

        static fifo<std::unique_ptr<const Events::ModifyGenerator::Request>,
            fifo_options::concurrency::single, // consumer
            fifo_options::concurrency::single, // producer
            fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // consumer
            fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // producer
            2> // max threads
            modifyGeneratorRequestQueue(REQUEST_QUEUE_SIZE);

        return modifyGeneratorRequestQueue;
    }

    // Get a reference to the oscillator modification result event queue, used for
    // passing messages about the status of an oscillator modification request from
    // the realtime thread to the non-realtime thread.
    static __forceinline auto& getModifyGeneratorResponseQueue()
    {
        static const int RESPONSE_QUEUE_SIZE = 32;
        static_assert(isPowerOf2(RESPONSE_QUEUE_SIZE));

        static fifo<Events::ModifyGenerator::Response,
            fifo_options::concurrency::single, // consumer
            fifo_options::concurrency::single, // producer
            fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // consumer
            fifo_options::full_empty_failure_mode::return_false_on_full_or_empty, // producer
            2> // max threads
            modifyGeneratorResponseQueue(RESPONSE_QUEUE_SIZE);

        return modifyGeneratorResponseQueue;
    }

    static __forceinline auto& getRealtimeAsyncCaller()
    {
        static const int QUEUE_SIZE = 512;
        static AsyncCaller<fifo_options::concurrency::single> asyncCaller(QUEUE_SIZE);
        return asyncCaller;
    }

    // Call on the realtime thread to defer execution to the non-realtime thread.
    // Useful for deleting memory or any other syscall-inducing functionality.
    bool deferToNonRealtimeThread(std::function<void()>&& fn)
    {
        return getRealtimeAsyncCaller().callAsync(std::move(fn));
    }

    // Call on the non-realtime thread to run deferred code.
    bool processDeferredActions()
    {
        return getRealtimeAsyncCaller().process();
    }
}

namespace GeneratorAccess
{
    // Get a reference to the generator.
    static __forceinline Generator<>& getInstance()
    {
        static Generator<> generator;
        return generator;
    }
};

// The functions in this namespace help the UI thread push events to
// the realtime thread via the modify generator request queue.
namespace EventBuilder
{
    bool PushAddOscillatorEvent(RequestId requestId, OscillatorSettings settings)
    {
        auto addOscillatorRequest = std::make_unique<Events::ModifyGenerator::AddOscillatorRequest>();
        addOscillatorRequest->id = requestId;
        addOscillatorRequest->action = Events::ModifyGenerator::Action::AddOscillator;
        addOscillatorRequest->settings = settings;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(addOscillatorRequest));
        assert(pushed);
        return pushed;
    }

    bool PushRemoveOscillatorEvent(RequestId requestId, OscillatorId idToRemove)
    {
        auto removeOscillatorRequest = std::make_unique<Events::ModifyGenerator::RemoveOscillatorRequest>();
        removeOscillatorRequest->id = requestId;
        removeOscillatorRequest->action = Events::ModifyGenerator::Action::RemoveOscillator;
        removeOscillatorRequest->idToRemove = idToRemove;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(removeOscillatorRequest));
        assert(pushed);
        return pushed;
    }

    bool PushActivateOscillatorEvent(RequestId requestId, OscillatorId idToModify, volume_t volume)
    {
        auto activateOscillatorRequest = std::make_unique<Events::ModifyGenerator::ActivateOscillatorRequest>();
        activateOscillatorRequest->action = Events::ModifyGenerator::Action::ActivateOscillator;
        activateOscillatorRequest->id = requestId;
        activateOscillatorRequest->idToModify = idToModify;
        activateOscillatorRequest->volume = volume;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(activateOscillatorRequest));
        assert(pushed);
        return pushed;
    }

    bool PushDeactivateOscillatorEvent(RequestId requestId, OscillatorId idToModify)
    {
        auto deactivateOscillatorRequest = std::make_unique<Events::ModifyGenerator::DeactivateOscillatorRequest>();
        deactivateOscillatorRequest->action = Events::ModifyGenerator::Action::DeactivateOscillator;
        deactivateOscillatorRequest->id = requestId;
        deactivateOscillatorRequest->idToModify = idToModify;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(deactivateOscillatorRequest));
        assert(pushed);
        return pushed;
    }

    bool PushSetOscillatorFrequencyEvent(RequestId requestId, OscillatorId idToModify, frequency_t frequency)
    {
        auto setOscillatorFrequencyRequest = std::make_unique<Events::ModifyGenerator::SetOscillatorFrequencyRequest>();
        setOscillatorFrequencyRequest->action = Events::ModifyGenerator::Action::SetOscillatorFrequency;
        setOscillatorFrequencyRequest->id = requestId;
        setOscillatorFrequencyRequest->idToModify = idToModify;
        setOscillatorFrequencyRequest->newFrequency = frequency;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(setOscillatorFrequencyRequest));
        assert(pushed);
        return pushed;
    }

    bool PushSetOscillatorVolumeEvent(RequestId requestId, OscillatorId idToModify, volume_t volume)
    {
        auto setOscillatorVolumeRequest = std::make_unique<Events::ModifyGenerator::SetOscillatorVolumeRequest>();
        setOscillatorVolumeRequest->action = Events::ModifyGenerator::Action::SetOscillatorVolume;
        setOscillatorVolumeRequest->id = requestId;
        setOscillatorVolumeRequest->idToModify = idToModify;
        setOscillatorVolumeRequest->newVolume = volume;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(setOscillatorVolumeRequest));
        assert(pushed);
        return pushed;
    }

    bool PushSetOscillatorPanEvent(RequestId requestId, OscillatorId idToModify, pan_t pan)
    {
        auto setOscillatorPanRequest = std::make_unique<Events::ModifyGenerator::SetOscillatorPanRequest>();
        setOscillatorPanRequest->action = Events::ModifyGenerator::Action::SetOscillatorPan;
        setOscillatorPanRequest->id = requestId;
        setOscillatorPanRequest->idToModify = idToModify;
        setOscillatorPanRequest->newPan = pan;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(setOscillatorPanRequest));
        assert(pushed);
        return pushed;
    }

    bool PushSetOscillatorTypeEvent(RequestId requestId, OscillatorId idToModify, OscillatorType type)
    {
        auto setOscillatorTypeRequest = std::make_unique<Events::ModifyGenerator::SetOscillatorTypeRequest>();
        setOscillatorTypeRequest->action = Events::ModifyGenerator::Action::SetOscillatorType;
        setOscillatorTypeRequest->id = requestId;
        setOscillatorTypeRequest->idToModify = idToModify;
        setOscillatorTypeRequest->newType = type;
        bool pushed = ThreadCommunication::getModifyGeneratorRequestQueue().push(std::move(setOscillatorTypeRequest));
        assert(pushed);
        return pushed;
    }
}

// TODO: add request type to params. add bool success to params. add request type to response. simplify ::result enum
//Events::ModifyGenerator::Response CreateResponse(RequestId requestId, OscillatorId oscillatorId)
//{
//    Events::ModifyGenerator::Response response;
//    response.requestId = requestId;
//    response.oscillatorId = oscillatorId;
//    return response;
//}


// These request handlers are meant to be called by the realtime thread.
// The realtime thread modifies the generator settings, honoring the
// request as best it can. It then responds informing how the event went.
// This response is used by the UI thread to keep the UI in sync.

bool HandleAddOscillatorRequest(const Events::ModifyGenerator::AddOscillatorRequest& addRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    Events::ModifyGenerator::Response addResponse;
    addResponse.requestId = addRequest.id;
    addResponse.oscillatorId = oscillators.addOscillator(addRequest.settings);
    addResponse.oscillatorSettings = addRequest.settings;
    addResponse.result = addResponse.oscillatorId.has_value() ?
        Events::ModifyGenerator::Result::AddOscillatorSucceeded :
        Events::ModifyGenerator::Result::AddOscillatorFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(addResponse));
}

bool HandleRemoveOscillatorRequest(const Events::ModifyGenerator::RemoveOscillatorRequest& removeRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    Events::ModifyGenerator::Response removeResponse;
    bool result = oscillators.removeOscillator(removeRequest.idToRemove);
    removeResponse.requestId = removeRequest.id;
    removeResponse.oscillatorId = removeRequest.idToRemove;
    removeResponse.result = result ?
        Events::ModifyGenerator::Result::RemoveOscillatorSucceeded :
        Events::ModifyGenerator::Result::RemoveOscillatorFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(removeResponse));
}

bool HandleActivateOscillatorRequest(const Events::ModifyGenerator::ActivateOscillatorRequest& activateRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    bool result = oscillators.activateOscillator(activateRequest.idToModify, activateRequest.volume);

    Events::ModifyGenerator::Response activateResponse;
    activateResponse.requestId = activateRequest.id;
    activateResponse.oscillatorId = activateRequest.idToModify;
    activateResponse.volume = activateRequest.volume;
    activateResponse.result = result ?
        Events::ModifyGenerator::Result::ActivateOscillatorSucceeded :
        Events::ModifyGenerator::Result::ActivateOscillatorFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(activateResponse));
}

bool HandleDeactivateOscillatorRequest(const Events::ModifyGenerator::DeactivateOscillatorRequest& deactivateRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    bool result = oscillators.deactivateOscillator(deactivateRequest.idToModify);

    Events::ModifyGenerator::Response deactivateResponse;
    deactivateResponse.requestId = deactivateRequest.id;
    deactivateResponse.oscillatorId = deactivateRequest.idToModify;
    deactivateResponse.result = result ?
        Events::ModifyGenerator::Result::DeactivateOscillatorSucceeded :
        Events::ModifyGenerator::Result::DeactivateOscillatorFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(deactivateResponse));
}

bool HandleSetOscillatorFrequencyRequest(const Events::ModifyGenerator::SetOscillatorFrequencyRequest& setFrequencyRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    bool result = oscillators.setFrequency(setFrequencyRequest.idToModify, setFrequencyRequest.newFrequency);

    Events::ModifyGenerator::Response setFrequencyResponse;
    setFrequencyResponse.requestId = setFrequencyRequest.id;
    setFrequencyResponse.oscillatorId = setFrequencyRequest.idToModify;
    setFrequencyResponse.frequency = setFrequencyRequest.newFrequency;
    setFrequencyResponse.result = result ?
        Events::ModifyGenerator::Result::SetOscillatorFrequencySucceeded :
        Events::ModifyGenerator::Result::SetOscillatorFrequencyFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(setFrequencyResponse));
}

bool HandleSetOscillatorVolumeRequest(const Events::ModifyGenerator::SetOscillatorVolumeRequest& setVolumeRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    bool result = oscillators.setVolume(setVolumeRequest.idToModify, setVolumeRequest.newVolume);

    Events::ModifyGenerator::Response setVolumeResponse;
    setVolumeResponse.requestId = setVolumeRequest.id;
    setVolumeResponse.oscillatorId = setVolumeRequest.idToModify;
    setVolumeResponse.volume = setVolumeRequest.newVolume;
    setVolumeResponse.result = result ?
        Events::ModifyGenerator::Result::SetOscillatorVolumeSucceeded :
        Events::ModifyGenerator::Result::SetOscillatorVolumeFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(setVolumeResponse));
}

bool HandleSetOscillatorPanRequest(const Events::ModifyGenerator::SetOscillatorPanRequest& setPanRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    bool result = oscillators.setPan(setPanRequest.idToModify, setPanRequest.newPan);

    Events::ModifyGenerator::Response setPanResponse;
    setPanResponse.requestId = setPanRequest.id;
    setPanResponse.oscillatorId = setPanRequest.idToModify;
    setPanResponse.pan = setPanRequest.newPan;
    setPanResponse.result = result ?
        Events::ModifyGenerator::Result::SetOscillatorPanSucceeded :
        Events::ModifyGenerator::Result::SetOscillatorPanFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(setPanResponse));
}

bool HandleSetOscillatorTypeRequest(const Events::ModifyGenerator::SetOscillatorTypeRequest& setTypeRequest)
{
    auto& oscillators = GeneratorAccess::getInstance().getOscillators();

    bool result = oscillators.setType(setTypeRequest.idToModify, setTypeRequest.newType);

    Events::ModifyGenerator::Response setTypeResponse;
    setTypeResponse.requestId = setTypeRequest.id;
    setTypeResponse.oscillatorId = setTypeRequest.idToModify;
    setTypeResponse.type = setTypeRequest.newType;
    setTypeResponse.result = result ?
        Events::ModifyGenerator::Result::SetOscillatorTypeSucceeded :
        Events::ModifyGenerator::Result::SetOscillatorTypeFailed;
    return ThreadCommunication::getModifyGeneratorResponseQueue().push(std::move(setTypeResponse));
}

bool DispatchModifyGeneratorRequest(const Events::ModifyGenerator::Request& request)
{
    switch (request.action)
    {
    case Events::ModifyGenerator::Action::AddOscillator:
        return HandleAddOscillatorRequest(
            static_cast<const Events::ModifyGenerator::AddOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::RemoveOscillator:
        return HandleRemoveOscillatorRequest(
            static_cast<const Events::ModifyGenerator::RemoveOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::ActivateOscillator:
        return HandleActivateOscillatorRequest(
            static_cast<const Events::ModifyGenerator::ActivateOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::DeactivateOscillator:
        return HandleDeactivateOscillatorRequest(
            static_cast<const Events::ModifyGenerator::DeactivateOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorFrequency:
        return HandleSetOscillatorFrequencyRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorFrequencyRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorVolume:
        return HandleSetOscillatorVolumeRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorVolumeRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorPan:
        return HandleSetOscillatorPanRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorPanRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorType:
        return HandleSetOscillatorTypeRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorTypeRequest&>(request));
    }

    assert(false); // unhandled request type!
    return false;
}

// Read from the generator request queue; handle all requests.
// Respond to each request to alert the UI thread what happened.
// This function is meant to be called by the realtime thread;
// it is in charge of honoring requests that it modify its settings.
void ProcessModifyGeneratorRequests()
{
    auto& requestQueue = ThreadCommunication::getModifyGeneratorRequestQueue();
    while (true)
    {
        // Create a new unique pointer every time. If there are multiple requests,
        // we need to schedule each of them for deletion individually.
        std::unique_ptr<const Events::ModifyGenerator::Request> request;
        if (!requestQueue.pop(request))
            break;

        bool dispatched = DispatchModifyGeneratorRequest(*request.get());
        assert(dispatched);
        unused(dispatched);

        // Delete the event later, on a non-realtime thread. No system calls on this thread.
        ThreadCommunication::deferToNonRealtimeThread(
            [requestPtr = request.release()]() { decltype(request) destructMe(requestPtr); });
    }
}

std::queue<RequestId>& GetRequestIds()
{
    static std::queue<RequestId> requestIds;
    return requestIds;
}

std::unordered_map<OscillatorId, OscillatorSettings>& GetUIOscillatorView()
{
    // Updated when a response comes back successfully and only then.
    static std::unordered_map<OscillatorId, OscillatorSettings> uiOscillatorView;
    return uiOscillatorView;
}

// TODO: this needs to be managed by some GUI-related class
// it should manage the GUI's view of the oscillator state
RequestId GetNextRequestId()
{
    static RequestId currentRequestId = 0;
    return currentRequestId++;
};

// This function, called from the non-realtime thread, checks for responses
// to requests sent in prior frames. Responses must come back in the same
// order that they were sent, and they must have the expected response values.
void HandleRealTimeThreadResponse()
{
    Events::ModifyGenerator::Response response;
    if (ThreadCommunication::getModifyGeneratorResponseQueue().pop(response))
    {
        auto& requestIds = GetRequestIds();
        auto& uiOscillatorView = GetUIOscillatorView();

        // Verify that the response responds to the expected request.
        const RequestId expectedResponseRequestId = requestIds.front();
        requestIds.pop();
        assert(response.requestId == expectedResponseRequestId);
        unused(expectedResponseRequestId);

        switch (response.result)
        {
        case Events::ModifyGenerator::Result::AddOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.oscillatorSettings.has_value());
            uiOscillatorView[*response.oscillatorId] = *response.oscillatorSettings;
            break;
        case Events::ModifyGenerator::Result::AddOscillatorFailed:
            //assert(false); // can't add any more oscillators! should disallow this in UI eventually.
            break;
        case Events::ModifyGenerator::Result::RemoveOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView.erase(*response.oscillatorId);
            break;
        case Events::ModifyGenerator::Result::RemoveOscillatorFailed:
            assert(false); // this is bad; we tried to remove an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::ActivateOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.volume.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView[*response.oscillatorId].state = OscillatorState::Active;
            uiOscillatorView[*response.oscillatorId].volume = *response.volume;
            break;
        case Events::ModifyGenerator::Result::ActivateOscillatorFailed:
            assert(false); // this is bad; we tried to activate an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::DeactivateOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView[*response.oscillatorId].state = OscillatorState::Deactivated;
            break;
        case Events::ModifyGenerator::Result::DeactivateOscillatorFailed:
            assert(false); // this is bad; we tried to deactivate an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorFrequencySucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.frequency.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView[*response.oscillatorId].frequency = *response.frequency;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorFrequencyFailed:
            assert(false); // this is bad; we tried to set the frequency of an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorVolumeSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.volume.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView[*response.oscillatorId].volume = *response.volume;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorVolumeFailed:
            assert(false); // this is bad; we tried to set the volume of an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorPanSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.pan.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView[*response.oscillatorId].pan = *response.pan;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorPanFailed:
            assert(false); // this is bad; we tried to set the pan of an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorTypeSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.type.has_value());
            assert(uiOscillatorView.contains(*response.oscillatorId));
            uiOscillatorView[*response.oscillatorId].type = *response.type;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorTypeFailed:
            assert(false); // this is bad; we tried to set the type of an oscillator that didn't exist. someone's confused.
            break;
        }
    }
}
