#include "plotting.h"

#include "util.h"

#include "implot.h"

#include <cmath>

namespace Plotting
{

void DrawOscilatorPlot(const std::vector<float>& logBufferL, const std::vector<float>& logBufferR)
{
    static bool paused = false;
    ImGui::Checkbox("Paused", &paused);

    static float t = 0.0f;
    static float lastTime = 0.0f;
    static ScrollingBuffer<50000> sdata1, sdata2;
    static size_t nextIndexToGraphL = 0;
    static size_t nextIndexToGraphR = 0;
    lastTime = t;

    if (!paused)
    {
        t += ImGui::GetIO().DeltaTime;

        const size_t diffL = logBufferL.size() - nextIndexToGraphL;
        size_t indexCounterL = 0;
        while (nextIndexToGraphL < logBufferL.size())
        {
            sdata1.addPoint(std::lerp(lastTime, t, float(indexCounterL) / diffL), logBufferL[nextIndexToGraphL]);
            nextIndexToGraphL++;
            indexCounterL++;
        }

        const size_t diffR = logBufferR.size() - nextIndexToGraphR;
        size_t indexCounterR = 0;
        while (nextIndexToGraphR < logBufferR.size())
        {
            sdata2.addPoint(std::lerp(lastTime, t, float(indexCounterR) / diffR), logBufferR[nextIndexToGraphR]);
            nextIndexToGraphR++;
            indexCounterR++;
        }
    }

    static float history = 3.0f;
    ImGui::SliderFloat("History", &history, .001f, 3.0f, "%.2f s", ImGuiSliderFlags_Logarithmic);

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 150))) {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0f, 1.0f);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        ImPlot::PlotLine("L", &sdata1.buffer[0].x, &sdata1.buffer[0].y, int(sdata1.buffer.size()), ImPlotLineFlags_None, int(sdata1.offset), sizeof(ImVec2));
        ImPlot::PlotLine("R", &sdata2.buffer[0].x, &sdata2.buffer[0].y, int(sdata2.buffer.size()), ImPlotLineFlags_None, int(sdata2.offset), sizeof(ImVec2));
        ImPlot::EndPlot();
    }
}

}
