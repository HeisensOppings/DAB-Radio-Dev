#include "./basic_radio_view_controller.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <stdint.h>
#include <vector>
#include <fmt/format.h>
#include "basic_radio/basic_audio_channel.h"
#include "basic_radio/basic_radio.h"
#include "dab/dab_misc_info.h"
#include "dab/database/dab_database.h"
#include "dab/database/dab_database_entities.h"
#include "dab/database/dab_database_types.h"
#include "dab/database/dab_database_updater.h"
#include "./formatters.h"
#include "./render_common.h"

template <typename T, typename F>
static T* find_by_callback(std::vector<T>& vec, F&& func) {
    for (auto& e: vec) {
        if (func(e)) return &e;
    }
    return nullptr;
}

// Render a list of all subchannels
void RenderSubchannels(BasicRadio& radio) {
    auto& db = radio.GetDatabase();
    auto window_label = fmt::format("Subchannels ({})###Subchannels Full List", db.subchannels.size());
    if (ImGui::Begin(window_label.c_str())) {
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (ImGui::BeginTable("Subchannels table", 6, flags)) 
        {
            ImGui::TableSetupColumn("Service Label",    ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("ID",               ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Start Address",    ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Capacity Units",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Protection",       ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Bitrate",          ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            int row_id  = 0;
            for (auto& subchannel: db.subchannels) {
                auto* service_component = find_by_callback(
                    db.service_components, 
                    [&subchannel](const auto& e) { 
                        return e.subchannel_id == subchannel.id; 
                    }
                );
                Service* service = nullptr;
                if (service_component) {
                    service = find_by_callback(
                        db.services,
                        [&service_component](const auto& e) {
                            return e.id.get_unique_identifier() == service_component->service_id.get_unique_identifier();
                        }
                    );
                }
                auto service_label = service ? service->label.c_str() : "";

                const auto prot_label = GetSubchannelProtectionLabel(subchannel);
                const uint32_t bitrate_kbps = GetSubchannelBitrate(subchannel);

                ImGui::PushID(row_id++);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextWrapped("%s", service_label);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextWrapped("0x%02X", subchannel.id);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextWrapped("%u", subchannel.start_address);
                ImGui::TableSetColumnIndex(3);
                ImGui::TextWrapped("%u", subchannel.length);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextWrapped("%.*s", int(prot_label.length()), prot_label.c_str());
                ImGui::TableSetColumnIndex(5);
                ImGui::TextWrapped("%u kb/s", bitrate_kbps);

                auto* dab_plus_channel = radio.Get_Audio_Channel(subchannel.id);
                if (dab_plus_channel != nullptr) {
                    auto& controls = dab_plus_channel->GetControls();
                    const bool is_selected = controls.GetAllEnabled();
                    ImGui::SameLine();
                    if (ImGui::Selectable("###select_button", is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                        if (is_selected) {
                            controls.StopAll();
                        } else {
                            controls.RunAll();
                        }
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

// Render the ensemble information
void RenderEnsemble(BasicRadio& radio) {
    if (ImGui::Begin("Ensemble")) {
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (ImGui::BeginTable("Ensemble description", 2, flags)) {
            ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            #define FIELD_MACRO(name, fmt, ...) {\
                ImGui::PushID(row_id++);\
                ImGui::TableNextRow();\
                ImGui::TableSetColumnIndex(0);\
                ImGui::TextWrapped(name);\
                ImGui::TableSetColumnIndex(1);\
                ImGui::TextWrapped(fmt, __VA_ARGS__);\
                ImGui::PopID();\
            }\

            #define HELP_MARKER(markerName, desc) {\
                ImGui::TextDisabled("%s", markerName);\
                if (ImGui::BeginItemTooltip())\
                {\
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);\
                    ImGui::TextUnformatted(desc);\
                    ImGui::PopTextWrapPos();\
                    ImGui::EndTooltip();\
                }\
            }\

            int row_id = 0;
            auto& db = radio.GetDatabase();
            auto& ensemble = db.ensemble;
            const float LTO = float(ensemble.local_time_offset) / 10.0f;
            FIELD_MACRO("Name", "%.*s", int(ensemble.label.length()), ensemble.label.c_str());
            FIELD_MACRO("Short Name", "%.*s", int(ensemble.short_label.length()), ensemble.short_label.c_str());
            FIELD_MACRO("Extended Name", "%.*s", int(ensemble.extended_label.label.length()), ensemble.extended_label.label.c_str());
            FIELD_MACRO("ID", "0x%04X", ensemble.id.get_unique_identifier());
            FIELD_MACRO("Country", "%s (0x%02X.%01X)", 
                GetCountryString(ensemble.extended_country_code, ensemble.id.get_country_code()),
                ensemble.extended_country_code, ensemble.id.get_country_code());
            FIELD_MACRO("Local Time Offset", "%.1f hours", LTO);
            FIELD_MACRO("Inter Table ID", "%u", ensemble.international_table_id);
            FIELD_MACRO("Total Services", "%u", ensemble.nb_services);
            FIELD_MACRO("Total Reconfig", "%u", ensemble.reconfiguration_count);
            #undef FIELD_MACRO

            ImGui::EndTable();

            ImGui::SeparatorText("Ensemble Information");
            ImGui::SetWindowFontScale(0.8f);
            auto BeginTableWithHeaders = [&](const char* id,
                                            std::initializer_list<const char*> headers,
                                            ImGuiTableFlags flags = ImGuiTableFlags_Borders 
                                                                | ImGuiTableFlags_RowBg 
                                                                | ImGuiTableFlags_Resizable) -> bool
            {
                if (ImGui::BeginTable(id, (int)headers.size(), flags)) {
                    for (auto* name : headers)
                        ImGui::TableSetupColumn(name);
                    ImGui::TableHeadersRow();
                    return true;
                }
                return false;
            };
            auto &Services = db.services;
            auto &ServiceComponents = db.service_components;
            auto &Subchannels = db.subchannels;
            if (ImGui::TreeNodeEx("Services", ImGuiTreeNodeFlags_DefaultOpen)) {
                HELP_MARKER("(Country?)", "See ts_101756 Table 3~7");
                ImGui::SameLine();
                HELP_MARKER("(ProgramType?)", "See ts_101756 Table 12~13");
                if (BeginTableWithHeaders("services", {"SId","Label","ShortLabel","Country","ProgramType"})) {
                    for (auto& s : db.services) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%u", s.id.value);
                        ImGui::TableNextColumn(); ImGui::Text("%s", s.label.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", s.short_label.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%d", s.id.get_country_code());
                        ImGui::TableNextColumn(); ImGui::Text("%d", s.programme_type);
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
                HELP_MARKER("(TMid?)", "Transport Mechanism Identifier. See en_300401 Figure 21\nTMId=00 (MSC stream audio)\nTMId=01 (MSC stream data)\nTMId=11 (MSC packet data)");
                ImGui::SameLine();
                HELP_MARKER("(ASCTy/DSCTy?)", "Audio and Data Service Component Type. See ts_101756 Table 2a~2b");
                ImGui::SameLine();
                HELP_MARKER("(UATy?)", "User Application Types. See ts_101756 Table 16");
                if (BeginTableWithHeaders("Components", {"TMid", "SId", "SCIdS", "SubChId", "ASCTy", "DSCTy", "UATy", "SCId", "PacketAddress"})) {
                    for (auto &comp : ServiceComponents)
                    {
                        auto appTypesStr = [&]() {
                            std::string s;
                            for (auto v : comp.application_types) {
                                s += std::to_string((int)v) + " ";
                            }
                            return s;
                        }();
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%d", (int)comp.transport_mode);
                        ImGui::TableNextColumn(); ImGui::Text("%u", comp.service_id.value);
                        ImGui::TableNextColumn(); ImGui::Text("%d", comp.component_id);
                        ImGui::TableNextColumn(); ImGui::Text("%d", (int)comp.subchannel_id);
                        ImGui::TableSetColumnIndex(6);
                        ImGui::Text("%s", appTypesStr.c_str());
                        if (comp.transport_mode == TransportMode::STREAM_MODE_AUDIO)
                        {
                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%d", (int)comp.audio_service_type);
                        }
                        else if (comp.transport_mode == TransportMode::STREAM_MODE_DATA)
                        {
                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%d", (int)comp.data_service_type);
                        }
                        else if (comp.transport_mode == TransportMode::PACKET_MODE_DATA)
                        {
                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%d", (int)comp.data_service_type);
                            ImGui::TableSetColumnIndex(7);
                            ImGui::Text("%d", (int)comp.global_id);
                            ImGui::TableSetColumnIndex(8);
                            ImGui::Text("%d", (int)comp.packet_address);
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("SubChannels", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (BeginTableWithHeaders("SubChannels", {"subChId", "startCU", "numCU", "fecScheme","protection", "bitrate(kbit/s)"})) {
                    for (const auto &subch : Subchannels)
                    {
                        const auto prot_label = GetSubchannelProtectionLabel(subch);
                        const uint32_t bitrate_kbps = GetSubchannelBitrate(subch);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%d", subch.id);
                        ImGui::TableNextColumn(); ImGui::Text("%d", subch.start_address);
                        ImGui::TableNextColumn(); ImGui::Text("%d", subch.length);
                        ImGui::TableNextColumn();
                        if(subch.fec_scheme != FEC_Scheme::UNDEFINED)
                            ImGui::Text("%d", (int)subch.fec_scheme);
                        ImGui::TableNextColumn(); ImGui::Text("%.*s", int(prot_label.length()), prot_label.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%u", bitrate_kbps);
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
            ImGui::SetWindowFontScale(1.0f);
        }
    }
    ImGui::End();
}

// Render misc information about the date and time
void RenderDateTime(BasicRadio& radio) {
    if (ImGui::Begin("Date & Time")) {
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (ImGui::BeginTable("Date & Time", 2, flags)) {
            ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            #define FIELD_MACRO(name, fmt, ...) {\
                ImGui::PushID(row_id++);\
                ImGui::TableNextRow();\
                ImGui::TableSetColumnIndex(0);\
                ImGui::TextWrapped(name);\
                ImGui::TableSetColumnIndex(1);\
                ImGui::TextWrapped(fmt, __VA_ARGS__);\
                ImGui::PopID();\
            }\

            int row_id = 0;
            const auto& info = radio.GetMiscInfo();
            FIELD_MACRO("Date", "%02d/%02d/%04d", 
                info.datetime.day, info.datetime.month, info.datetime.year);
            FIELD_MACRO("Time", "%02u:%02u:%02u.%03u", 
                info.datetime.hours, info.datetime.minutes, 
                info.datetime.seconds, info.datetime.milliseconds);
            FIELD_MACRO("CIF Counter", "%4u = %2u|%-3u", 
                info.cif_counter.GetTotalCount(),
                info.cif_counter.upper_count, info.cif_counter.lower_count);

            #undef FIELD_MACRO
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

// Database statistics
void RenderDatabaseStatistics(BasicRadio& radio) {
    if (ImGui::Begin("Database Stats")) {
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (ImGui::BeginTable("Date & Time", 2, flags)) {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            #define FIELD_MACRO(name, fmt, ...) {\
                ImGui::PushID(row_id++);\
                ImGui::TableNextRow();\
                ImGui::TableSetColumnIndex(0);\
                ImGui::TextWrapped(name);\
                ImGui::TableSetColumnIndex(1);\
                ImGui::TextWrapped(fmt, __VA_ARGS__);\
                ImGui::PopID();\
            }\

            int row_id = 0;
            const auto& stats = radio.GetDatabaseStatistics();
            FIELD_MACRO("Total", "%zu", stats.nb_total);
            FIELD_MACRO("Pending", "%zu", stats.nb_pending);
            FIELD_MACRO("Completed", "%zu", stats.nb_completed);
            FIELD_MACRO("Conflicts", "%zu", stats.nb_conflicts);
            FIELD_MACRO("Updates", "%zu", stats.nb_updates);

            #undef FIELD_MACRO
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

// Linked ensembles
void RenderOtherEnsembles(BasicRadio& radio) {
    auto& db = radio.GetDatabase();
    auto label = fmt::format("Other Ensembles ({})###Other Ensembles", db.other_ensembles.size());

    const auto ensemble = db.ensemble;

    if (ImGui::Begin(label.c_str())) {
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders;
        if (ImGui::BeginTable("Components table", 6, flags)) 
        {
            ImGui::TableSetupColumn("ID",                       ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Country",                  ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Continuous Output",        ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Geographically Adjacent",  ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Mode I",                   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Frequency",                ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            int row_id  = 0;
            for (const auto& other_ensemble: db.other_ensembles) {
                ImGui::PushID(row_id++);

                const float frequency =  static_cast<float>(other_ensemble.frequency) * 1e-6f;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextWrapped("0x%04X", other_ensemble.id.get_unique_identifier());
                ImGui::TableSetColumnIndex(1);

                // assume that the other ensemble is in the same region as the current ensemble
                const extended_country_id_t extended_country_code = ensemble.extended_country_code;
                country_id_t country_code = other_ensemble.id.get_country_code();
                if (country_code == 0) {
                    country_code = ensemble.id.get_country_code();
                }
                ImGui::TextWrapped("%s (0x%02X.%01X)",
                    GetCountryString(extended_country_code, country_code),
                    extended_country_code, country_code
                );
                ImGui::TableSetColumnIndex(2);
                ImGui::TextWrapped("%s", other_ensemble.is_continuous_output ? "Yes" : "No");
                ImGui::TableSetColumnIndex(3);
                ImGui::TextWrapped("%s", other_ensemble.is_geographically_adjacent ? "Yes" : "No");
                ImGui::TableSetColumnIndex(4);
                ImGui::TextWrapped("%s", other_ensemble.is_transmission_mode_I ? "Yes" : "No");
                ImGui::TableSetColumnIndex(5);
                ImGui::TextWrapped("%3.3f MHz", frequency);

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

