
#include "Node.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <unordered_map>

inline void NodeGraphWindow(NodeGraph& graph) {
  auto style = ImGui::GetStyle();

  std::unordered_map<const Port*, ImVec2> portPositions;

  // Iterate through nodes and render them
  for (auto& node : graph.getNodes()) {
    // create a simple window for each node
    // the identifier/label is node label plus its unique ID to avoid conflicts
    std::string winIdentifier{node->label()};
    winIdentifier += "##";
    winIdentifier += std::to_string(node->id());
    ImGui::Begin(winIdentifier.c_str(), nullptr, 0);

    // Inputs in left column, outputs in right column
    ImGui::Columns(2, nullptr, false);
    for (const auto& inputName : node->inputNames()) {
      ImGui::Button(inputName.data());
      // get last item position
      auto pos = ImGui::GetItemRectMin() +
                 ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};
      portPositions[node->inputPort(inputName)] = pos;
    }
    ImGui::NextColumn();
    for (const auto& outputName : node->outputNames()) {
      float width = ImGui::CalcTextSize(outputName.data()).x +
                    style.FramePadding.x * 2 + style.ItemSpacing.x;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                           (ImGui::GetColumnWidth() - width));
      ImGui::Button(outputName.data());
      auto pos = ImGui::GetItemRectMin() + ImGui::GetItemRectSize() -
                 ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};
      portPositions[node->outputPort(outputName)] = pos;
    }
    ImGui::Columns(1);
    ImGui::Separator();

    // auto n{std::max(node->inputNames().size(), node->outputNames().size())};
    // for (std::size_t i = 0; i < n; ++i) {
    //   // Input ports
    //   ImGui::BeginGroup();
    //   if (i < node->inputNames().size()) {
    //     auto inputName{node->inputNames()[i]};
    //     ImGui::Text("%s", inputName.data());
    //     // get last item position
    //     auto pos = ImGui::GetItemRectMin() +
    //                ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};
    //     portPositions[node->inputPort(inputName)] = pos;
    //   } else {
    //     ImGui::Text("");
    //   }
    //   ImGui::EndGroup();

    //   // Output ports
    //   ImGui::SameLine(100.0f);
    //   if (i < node->outputNames().size()) {
    //     auto outputName{node->outputNames()[i]};
    //     ImGui::Text("%s", outputName.data());
    //     auto pos = ImGui::GetItemRectMin() + ImGui::GetItemRectSize() -
    //                ImVec2{0.0f, ImGui::GetTextLineHeight() / 2};
    //     portPositions[node->outputPort(outputName)] = pos;
    //   } else {
    //     ImGui::Text("");
    //   }

    //   ImGui::NewLine();
    // }

    // Render node-specific UI
    node->render();

    ImGui::End();
  }

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  // nice flashy red color for the links
  const ImU32 linkColor     = IM_COL32(255, 100, 100, 255);
  const float linkThickness = 3.0f;

  for (auto& node : graph.getNodes()) {
    for (auto& inputName : node->inputNames()) {
      const Port* inPort    = node->inputPort(inputName);
      const Port* connected = inPort->connected();
      if (!connected)
        continue;

      auto itA = portPositions.find(inPort);
      auto itB = portPositions.find(connected);
      if (itA != portPositions.end() && itB != portPositions.end()) {
        drawList->AddBezierCubic(itB->second, itB->second + ImVec2{50.0f, 0.0f},
                                 itA->second - ImVec2{50.0f, 0.0f}, itA->second,
                                 linkColor, linkThickness);
      }
    }
  }

  graph.update();
}