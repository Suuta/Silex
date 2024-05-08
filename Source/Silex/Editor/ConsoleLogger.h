#pragma once

#include "Core/Core.h"
#include <imgui/imgui.h>


namespace Silex
{
    class ConsoleLogger
    {
    public:

        static ConsoleLogger& Get();

        void Log(const std::string& message)
        {
            m_LogData += message;
        }

        void Log(LogLevel level, const std::string& message)
        {
            // 一定サイズなら、先頭要素を消す
            if (m_LogEntries.size() > m_MaxLogEntory)
            {
                m_LogEntries.pop_front();
            }

            LogEntry entry;
            entry.Level = level;
            entry.Text  = message;

            m_LogEntries.emplace_back(entry);
        }

        void Clear()
        {
            m_LogData.clear();

            // queue に clearメンバ関数が無いので空オブジェクトとスワップ
            //std::queue<LogEntry> empty;
            //std::swap(m_LogEntries, empty);

            m_LogEntries.clear();
        }

        const char* Data()
        {
            return m_LogData.c_str();
        }

        void LogData()
        {
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            for (auto& entry : m_LogEntries)
            {
                ImVec4 color;
                
                switch(entry.Level)
                {
                    case LogLevel::Fatal   : color = ImVec4(0.8f, 0.0f, 0.8f, 1.0f); break;
                    case LogLevel::Error   : color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); break;
                    case LogLevel::Warning : color = ImVec4(0.8f, 0.6f, 0.0f, 1.0f); break;
                    case LogLevel::Info    : color = ImVec4(0.0f, 0.6f, 0.0f, 1.0f); break;
                    case LogLevel::Trace   : color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
                    case LogLevel::Debug   : color = ImVec4(0.0f, 0.5f, 0.8f, 1.0f); break;
                
                    default: color = ImVec4(1, 1, 1, 1); break;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%s", entry.Text.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();
        }

    private:

        struct LogEntry
        {
            std::string Text;
            LogLevel    Level;
        };

    private:

        uint64 m_MaxLogEntory = 1024;

        std::deque<LogEntry> m_LogEntries;
        std::string          m_LogData;
    };
}

