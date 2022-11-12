#include "oscillator_ui.h"

RequestId UIOscillatorView::GetNextRequestId()
{
    // note: just let it wrap around to 0, should be fine :D
    return m_currentRequestId++;
};

void UIOscillatorView::HandleRealTimeResponse()
{
    Events::ModifyGenerator::Response response;
    if (ThreadCommunication::getModifyGeneratorResponseQueue().pop(response))
    {
        // Verify that the response responds to the expected request.
        auto& requestIds = GetRequestIds();
        const RequestId expectedResponseRequestId = requestIds.front();
        requestIds.pop();
        assert(response.requestId == expectedResponseRequestId);
        unused(expectedResponseRequestId);

        switch (response.result)
        {
        case Events::ModifyGenerator::Result::AddOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.oscillatorSettings.has_value());
            m_oscillators[*response.oscillatorId] = *response.oscillatorSettings;
            break;
        case Events::ModifyGenerator::Result::AddOscillatorFailed:
            //assert(false); // can't add any more oscillators! should disallow this in UI eventually.
            break;
        case Events::ModifyGenerator::Result::RemoveOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators.erase(*response.oscillatorId);
            break;
        case Events::ModifyGenerator::Result::RemoveOscillatorFailed:
            assert(false); // this is bad; we tried to remove an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::ActivateOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.volume.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators[*response.oscillatorId].state = OscillatorState::Active;
            m_oscillators[*response.oscillatorId].volume = *response.volume;
            break;
        case Events::ModifyGenerator::Result::ActivateOscillatorFailed:
            assert(false); // this is bad; we tried to activate an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::DeactivateOscillatorSucceeded:
            assert(response.oscillatorId.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators[*response.oscillatorId].state = OscillatorState::Deactivated;
            break;
        case Events::ModifyGenerator::Result::DeactivateOscillatorFailed:
            assert(false); // this is bad; we tried to deactivate an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorFrequencySucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.frequency.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators[*response.oscillatorId].frequency = *response.frequency;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorFrequencyFailed:
            assert(false); // this is bad; we tried to set the frequency of an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorVolumeSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.volume.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators[*response.oscillatorId].volume = *response.volume;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorVolumeFailed:
            assert(false); // this is bad; we tried to set the volume of an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorPanSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.pan.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators[*response.oscillatorId].pan = *response.pan;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorPanFailed:
            assert(false); // this is bad; we tried to set the pan of an oscillator that didn't exist. someone's confused.
            break;
        case Events::ModifyGenerator::Result::SetOscillatorTypeSucceeded:
            assert(response.oscillatorId.has_value());
            assert(response.type.has_value());
            assert(m_oscillators.contains(*response.oscillatorId));
            m_oscillators[*response.oscillatorId].type = *response.type;
            break;
        case Events::ModifyGenerator::Result::SetOscillatorTypeFailed:
            assert(false); // this is bad; we tried to set the type of an oscillator that didn't exist. someone's confused.
            break;
        }
    }
}

void UIOscillatorView::Show()
{
    auto& requestIds = GetRequestIds();
    auto& uiOscillatorView = GetUIOscillatorView();

    ImGui::Text("Adjust settings of the generator:");

    if (ImGui::Button("Add Oscillator"))
    {
        const RequestId requestId = uiOscillatorView.GetNextRequestId();
        requestIds.push(requestId);
        EventBuilder::PushAddOscillatorEvent(
            requestId,
            OscillatorSettings(OscillatorType::Sine, 200.f, .2f));
    }

    for (const auto& [oscillatorId, settings] : m_oscillators)
        ShowOscillator(oscillatorId, settings);
}

void UIOscillatorView::ShowOscillator(const OscillatorId& oscillatorId, const OscillatorSettings& settings)
{
    auto& requestIds = GetRequestIds();
    char removeLabel[100];
    sprintf_s(removeLabel, "Remove##%u", oscillatorId);
    if (ImGui::Button(removeLabel))
    {
        const RequestId requestId = GetNextRequestId();
        requestIds.push(requestId);
        EventBuilder::PushRemoveOscillatorEvent(requestId, oscillatorId);
    }

    ImGui::SameLine();
    bool activeBool = settings.state == OscillatorState::Active;
    char activeLabel[100];
    sprintf_s(activeLabel, "Active##%u", oscillatorId);
    if (ImGui::Checkbox(activeLabel, &activeBool))
    {
        const RequestId requestId = GetNextRequestId();
        requestIds.push(requestId);

        if (activeBool)
            EventBuilder::PushActivateOscillatorEvent(requestId, oscillatorId, settings.volume);
        else
            EventBuilder::PushDeactivateOscillatorEvent(requestId, oscillatorId);
    }

    ImGui::SameLine();
    const char* types[] = { "Sine", "Square", "Triangle", "Saw" };
    const int currentIndex = int(settings.type);
    const char* comboLabel = types[currentIndex];
    char typeLabel[100];
    sprintf_s(typeLabel, "Type##%u", oscillatorId);
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::BeginCombo(typeLabel, comboLabel))
    {
        for (int n = 0; n < IM_ARRAYSIZE(types); n++)
        {
            const bool isSelected = (currentIndex == n);
            if (isSelected)
                ImGui::SetItemDefaultFocus();

            if (ImGui::Selectable(types[n], isSelected))
            {
                if (isSelected)
                    break;

                const RequestId requestId = GetNextRequestId();
                requestIds.push(requestId);
                EventBuilder::PushSetOscillatorTypeEvent(requestId, oscillatorId, OscillatorType(n));
            }
        }
        ImGui::EndCombo();
    }

    volume_t volume = settings.volume;
    char volumeLabel[100];
    sprintf_s(volumeLabel, "Volume##%u", oscillatorId);
    if (ImGui::SliderFloat(volumeLabel, &volume, 0.0f, 1.0f))
    {
        const RequestId requestId = GetNextRequestId();
        requestIds.push(requestId);
        EventBuilder::PushSetOscillatorVolumeEvent(requestId, oscillatorId, volume);
    }

    pan_t pan = settings.pan;
    char panLabel[100];
    sprintf_s(panLabel, "Pan##%u", oscillatorId);
    if (ImGui::SliderFloat(panLabel, &pan, -1.0f, 1.0f))
    {
        const RequestId requestId = GetNextRequestId();
        requestIds.push(requestId);
        EventBuilder::PushSetOscillatorPanEvent(requestId, oscillatorId, pan);
    }

    frequency_t frequency = settings.frequency;
    char frequencyLabel[100];
    sprintf_s(frequencyLabel, "Frequency##%u", oscillatorId);
    if (ImGui::SliderFloat(frequencyLabel, &frequency, 20.0f, 8000.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
    {
        const RequestId requestId = GetNextRequestId();
        requestIds.push(requestId);
        EventBuilder::PushSetOscillatorFrequencyEvent(requestId, oscillatorId, frequency);
    }

    ImGui::NewLine();
}

