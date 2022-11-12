#include "thread_communication.h"

#include "oscillator_ui.h"

ThreadCommunication::RequestQueueType& ThreadCommunication::getModifyGeneratorRequestQueue()
{
    static const int REQUEST_QUEUE_SIZE = 32;
    static_assert(isPowerOf2(REQUEST_QUEUE_SIZE));
    static RequestQueueType modifyGeneratorRequestQueue(REQUEST_QUEUE_SIZE);
    return modifyGeneratorRequestQueue;
}

ThreadCommunication::ResponseQueueType& ThreadCommunication::getModifyGeneratorResponseQueue()
{
    static const int RESPONSE_QUEUE_SIZE = 32;
    static_assert(isPowerOf2(RESPONSE_QUEUE_SIZE));
    static ResponseQueueType modifyGeneratorResponseQueue(RESPONSE_QUEUE_SIZE);
    return modifyGeneratorResponseQueue;
}

ThreadCommunication::AsyncCallerType& ThreadCommunication::getRealtimeAsyncCaller()
{
    static const int QUEUE_SIZE = 512;
    static AsyncCallerType asyncCaller(QUEUE_SIZE);
    return asyncCaller;
}

bool ThreadCommunication::deferToNonRealtimeThread(std::function<void()>&& fn)
{
    return getRealtimeAsyncCaller().callAsync(std::move(fn));
}

bool ThreadCommunication::processDeferredActions()
{
    return getRealtimeAsyncCaller().process();
}


Generator<>& GeneratorAccess::getInstance()
{
    static Generator<> generator;
    return generator;
}

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

// These request handlers are meant to be called by the realtime thread.
// The realtime thread modifies the generator settings, honoring the
// request as best it can. It then responds informing how the event went.
// This response is used by the UI thread to keep the UI in sync.
namespace RealTimeRequestHandlers
{
    static bool HandleAddOscillatorRequest(const Events::ModifyGenerator::AddOscillatorRequest& addRequest)
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

    static bool HandleRemoveOscillatorRequest(const Events::ModifyGenerator::RemoveOscillatorRequest& removeRequest)
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

    static bool HandleActivateOscillatorRequest(const Events::ModifyGenerator::ActivateOscillatorRequest& activateRequest)
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

    static bool HandleDeactivateOscillatorRequest(const Events::ModifyGenerator::DeactivateOscillatorRequest& deactivateRequest)
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

    static bool HandleSetOscillatorFrequencyRequest(const Events::ModifyGenerator::SetOscillatorFrequencyRequest& setFrequencyRequest)
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

    static bool HandleSetOscillatorVolumeRequest(const Events::ModifyGenerator::SetOscillatorVolumeRequest& setVolumeRequest)
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

    static bool HandleSetOscillatorPanRequest(const Events::ModifyGenerator::SetOscillatorPanRequest& setPanRequest)
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

    static bool HandleSetOscillatorTypeRequest(const Events::ModifyGenerator::SetOscillatorTypeRequest& setTypeRequest)
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
}

bool DispatchModifyGeneratorRequest(const Events::ModifyGenerator::Request& request)
{
    switch (request.action)
    {
    case Events::ModifyGenerator::Action::AddOscillator:
        return RealTimeRequestHandlers::HandleAddOscillatorRequest(
            static_cast<const Events::ModifyGenerator::AddOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::RemoveOscillator:
        return RealTimeRequestHandlers::HandleRemoveOscillatorRequest(
            static_cast<const Events::ModifyGenerator::RemoveOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::ActivateOscillator:
        return RealTimeRequestHandlers::HandleActivateOscillatorRequest(
            static_cast<const Events::ModifyGenerator::ActivateOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::DeactivateOscillator:
        return RealTimeRequestHandlers::HandleDeactivateOscillatorRequest(
            static_cast<const Events::ModifyGenerator::DeactivateOscillatorRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorFrequency:
        return RealTimeRequestHandlers::HandleSetOscillatorFrequencyRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorFrequencyRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorVolume:
        return RealTimeRequestHandlers::HandleSetOscillatorVolumeRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorVolumeRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorPan:
        return RealTimeRequestHandlers::HandleSetOscillatorPanRequest(
            static_cast<const Events::ModifyGenerator::SetOscillatorPanRequest&>(request));
    case Events::ModifyGenerator::Action::SetOscillatorType:
        return RealTimeRequestHandlers::HandleSetOscillatorTypeRequest(
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

UIOscillatorView& GetUIOscillatorView()
{
    static UIOscillatorView uiOscillatorView;
    return uiOscillatorView;
}
