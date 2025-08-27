#include "./dab_database_updater.h"
#include <stdint.h>
#include <memory>
#include <string_view>
#include <vector>
#include "./dab_database.h"
#include "./dab_database_entities.h"
#include "./dab_database_types.h"

template <typename T>
bool insert_if_unique(std::vector<T>& vec, T value) {
    for (const auto& v: vec) {
        if (v == value) return false;
    }
    vec.push_back(value);
    return true;
}

// Ensemble form
const uint16_t ENSEMBLE_FLAG_ID             = 0b1000000000;
const uint16_t ENSEMBLE_FLAG_ECC            = 0b0100000000;
const uint16_t ENSEMBLE_FLAG_LABEL          = 0b0001000000;
const uint16_t ENSEMBLE_FLAG_SHORT_LABEL    = 0b0000100000;
const uint16_t ENSEMBLE_FLAG_NB_SERVICES    = 0b0000010000;
const uint16_t ENSEMBLE_FLAG_RCOUNT         = 0b0000001000;
const uint16_t ENSEMBLE_FLAG_LTO            = 0b0000000100;
const uint16_t ENSEMBLE_FLAG_INTER_TABLE    = 0b0000000010;
const uint16_t ENSEMBLE_FLAG_EXTENDED_LABEL = 0b0000000001;
const uint16_t ENSEMBLE_FLAG_REQUIRED       = 0b1000000010;

UpdateResult EnsembleUpdater::SetID(const EnsembleId ensemble_id) {
    return UpdateField(GetData().id, ensemble_id, ENSEMBLE_FLAG_ID);
}

UpdateResult EnsembleUpdater::SetExtendedCountryCode(const extended_country_id_t extended_country_code) {
    // 0x00 is a NULL extended country code
    // this occurs if the packet doesn't define it
    if (extended_country_code == 0x00) {
        return UpdateResult::NO_CHANGE;
    }
    return UpdateField(GetData().extended_country_code, extended_country_code, ENSEMBLE_FLAG_ECC);
}

UpdateResult EnsembleUpdater::SetLabel(std::string_view label) {
    return UpdateField(GetData().label, label, ENSEMBLE_FLAG_LABEL);
}

UpdateResult EnsembleUpdater::SetShortLabel(std::string_view short_label) {
    return UpdateField(GetData().short_label, short_label, ENSEMBLE_FLAG_SHORT_LABEL);
}

UpdateResult EnsembleUpdater::SetExtendedLabel(std::string_view extended_label) {
    return UpdateField(GetData().extended_label.label, extended_label, ENSEMBLE_FLAG_EXTENDED_LABEL);
}

UpdateResult EnsembleUpdater::SetNumberServices(const uint8_t nb_services) {
    return UpdateField(GetData().nb_services, nb_services, ENSEMBLE_FLAG_NB_SERVICES);
}

UpdateResult EnsembleUpdater::SetReconfigurationCount(const uint16_t reconfiguration_count) {
    return UpdateField(GetData().reconfiguration_count, reconfiguration_count, ENSEMBLE_FLAG_RCOUNT);
}

UpdateResult EnsembleUpdater::SetLocalTimeOffset(const int8_t local_time_offset) {
    return UpdateField(GetData().local_time_offset, local_time_offset, ENSEMBLE_FLAG_LTO);
}

UpdateResult EnsembleUpdater::SetInternationalTableID(const uint8_t international_table_id) {
    return UpdateField(GetData().international_table_id, international_table_id, ENSEMBLE_FLAG_INTER_TABLE);
}

bool EnsembleUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & ENSEMBLE_FLAG_REQUIRED) == ENSEMBLE_FLAG_REQUIRED);
}

// Service form
const uint8_t SERVICE_FLAG_LABEL            = 0b00100000;
const uint8_t SERVICE_FLAG_PROGRAM_TYPE     = 0b00010000;
const uint8_t SERVICE_FLAG_SHORT_LABEL      = 0b00001000;
const uint8_t SERVICE_FLAG_EXTENDED_LABEL   = 0b00000100;
const uint8_t SERVICE_FLAG_ASU_FLAG         = 0b00000010;
const uint8_t SERVICE_FLAG_CLUSTER_IDS      = 0b00000001;
const uint8_t SERVICE_FLAG_REQUIRED         = 0b00000000;

UpdateResult ServiceUpdater::SetLabel(std::string_view label) {
    return UpdateField(GetData().label, label, SERVICE_FLAG_LABEL);
}

UpdateResult ServiceUpdater::SetShortLabel(std::string_view short_label) {
    return UpdateField(GetData().short_label, short_label, SERVICE_FLAG_SHORT_LABEL);
}

UpdateResult ServiceUpdater::SetExtendedLabel(std::string_view extended_label) {
    return UpdateField(GetData().extended_label.label, extended_label, SERVICE_FLAG_EXTENDED_LABEL);
}

UpdateResult ServiceUpdater::SetProgrammeType(const programme_id_t programme_type) {
    return UpdateField(GetData().programme_type, programme_type, SERVICE_FLAG_PROGRAM_TYPE);
}

UpdateResult ServiceUpdater::SetASuFlags(const asu_flags_t asu_flags)
{
    return UpdateField(GetData().asu_flags, asu_flags, SERVICE_FLAG_ASU_FLAG);
}

UpdateResult ServiceUpdater::AddClusterID(const cluster_id_t cluster_id)
{
    if (!insert_if_unique(GetData().cluster_ids, cluster_id)) return UpdateResult::NO_CHANGE;
    m_dirty_field |= SERVICE_FLAG_CLUSTER_IDS;
    OnComplete();
    OnUpdate();
    return UpdateResult::SUCCESS;
}

bool ServiceUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & SERVICE_FLAG_REQUIRED) == SERVICE_FLAG_REQUIRED);
}

// Service component form
const uint16_t SERVICE_COMPONENT_FLAG_COMPONENT_ID          = 0b1000000000000;
const uint16_t SERVICE_COMPONENT_FLAG_LABEL                 = 0b0100000000000;
const uint16_t SERVICE_COMPONENT_FLAG_TRANSPORT_MODE        = 0b0010000000000;
const uint16_t SERVICE_COMPONENT_FLAG_AUDIO_TYPE            = 0b0001000000000;
const uint16_t SERVICE_COMPONENT_FLAG_DATA_TYPE             = 0b0000100000000;
const uint16_t SERVICE_COMPONENT_FLAG_SUBCHANNEL            = 0b0000010000000;
const uint16_t SERVICE_COMPONENT_FLAG_GLOBAL_ID             = 0b0000001000000;
const uint16_t SERVICE_COMPONENT_FLAG_SHORT_LABEL           = 0b0000000100000;
const uint16_t SERVICE_COMPONENT_FLAG_PACKET_ADDRESS        = 0b0000000010000;
const uint16_t SERVICE_COMPONENT_FLAG_DG_FLAG               = 0b0000000001000;
const uint16_t SERVICE_COMPONENT_FLAG_LANGUAGE              = 0b0000000000100;
const uint16_t SERVICE_COMPONENT_FLAG_APPLICATION_TYPE      = 0b0000000000010;
const uint16_t SERVICE_COMPONENT_FLAG_EXTENDED_LABEL        = 0b0000000000001;
// A different set of required fields applies to stream audio, stream data, and packet data components.
const uint16_t SERVICE_COMPONENT_FLAG_REQUIRED_STREAM_AUDIO = 0b0011010000000;
const uint16_t SERVICE_COMPONENT_FLAG_REQUIRED_STREAM_DATA  = 0b0010110000000;
const uint16_t SERVICE_COMPONENT_FLAG_REQUIRED_PACKET_DATA  = 0b0010110011010;

UpdateResult ServiceComponentUpdater::SetLabel(std::string_view label) {
    return UpdateField(GetData().label, label, SERVICE_COMPONENT_FLAG_LABEL);
}

UpdateResult ServiceComponentUpdater::SetShortLabel(std::string_view short_label) {
    return UpdateField(GetData().short_label, short_label, SERVICE_COMPONENT_FLAG_SHORT_LABEL);
}

UpdateResult ServiceComponentUpdater::SetExtendedLabel(std::string_view extended_label) {
    return UpdateField(GetData().extended_label.label, extended_label, SERVICE_COMPONENT_FLAG_EXTENDED_LABEL);
}

UpdateResult ServiceComponentUpdater::SetTransportMode(const TransportMode transport_mode) {
    return UpdateField(GetData().transport_mode, transport_mode, SERVICE_COMPONENT_FLAG_TRANSPORT_MODE);
}

UpdateResult ServiceComponentUpdater::SetAudioServiceType(const AudioServiceType audio_service_type) {
    if (m_dirty_field & SERVICE_COMPONENT_FLAG_DATA_TYPE) {
        OnConflict();
        return UpdateResult::CONFLICT;
    }
    return UpdateField(GetData().audio_service_type, audio_service_type, SERVICE_COMPONENT_FLAG_AUDIO_TYPE);
}

UpdateResult ServiceComponentUpdater::SetDataServiceType(const DataServiceType data_service_type) {
    if (m_dirty_field & SERVICE_COMPONENT_FLAG_AUDIO_TYPE) {
        OnConflict();
        return UpdateResult::CONFLICT;
    }
    return UpdateField(GetData().data_service_type, data_service_type, SERVICE_COMPONENT_FLAG_DATA_TYPE);
}

UpdateResult ServiceComponentUpdater::SetSubchannel(const subchannel_id_t subchannel_id) {
    return UpdateField(GetData().subchannel_id, subchannel_id, SERVICE_COMPONENT_FLAG_SUBCHANNEL);
}

UpdateResult ServiceComponentUpdater::SetPacketAddr(const packet_addr_t packet_addr) {
    return UpdateField(GetData().packet_address, packet_addr, SERVICE_COMPONENT_FLAG_PACKET_ADDRESS);
}

UpdateResult ServiceComponentUpdater::SetDGFlag(const dg_flag_t dg_flag) {
    return UpdateField(GetData().dg_flag, dg_flag, SERVICE_COMPONENT_FLAG_DG_FLAG);
}

UpdateResult ServiceComponentUpdater::SetLanguage(const language_id_t language) {
    return UpdateField(GetData().language, language, SERVICE_COMPONENT_FLAG_LANGUAGE);
}

UpdateResult ServiceComponentUpdater::AddUserApplicationType(const user_application_type_t application_type) {
    if (!insert_if_unique(GetData().application_types, application_type)) return UpdateResult::NO_CHANGE;
    m_dirty_field |= SERVICE_COMPONENT_FLAG_APPLICATION_TYPE;
    OnComplete();
    OnUpdate();
    return UpdateResult::SUCCESS;
}

UpdateResult ServiceComponentUpdater::SetComponentID(const service_component_id_t component_id) {
    return UpdateField(GetData().component_id, component_id, SERVICE_COMPONENT_FLAG_COMPONENT_ID);
}

UpdateResult ServiceComponentUpdater::SetGlobalID(const service_component_global_id_t global_id) {
    return UpdateField(GetData().global_id, global_id, SERVICE_COMPONENT_FLAG_GLOBAL_ID);
}

bool ServiceComponentUpdater::IsComplete() {
    const bool stream_audio_complete = (m_dirty_field & SERVICE_COMPONENT_FLAG_REQUIRED_STREAM_AUDIO) == SERVICE_COMPONENT_FLAG_REQUIRED_STREAM_AUDIO;
    const bool stream_data_complete  = (m_dirty_field & SERVICE_COMPONENT_FLAG_REQUIRED_STREAM_DATA) == SERVICE_COMPONENT_FLAG_REQUIRED_STREAM_DATA;
    const bool packet_data_complete  = (m_dirty_field & SERVICE_COMPONENT_FLAG_REQUIRED_PACKET_DATA) == SERVICE_COMPONENT_FLAG_REQUIRED_PACKET_DATA;
    const bool is_complete = (GetData().transport_mode == TransportMode::STREAM_MODE_AUDIO) ? stream_audio_complete : 
                            ((GetData().transport_mode == TransportMode::STREAM_MODE_DATA) ? stream_data_complete : packet_data_complete);
    GetData().is_complete = is_complete;
    return is_complete;
}

// Subchannel form
const uint8_t SUBCHANNEL_FLAG_START_ADDRESS     = 0b10000000;
const uint8_t SUBCHANNEL_FLAG_LENGTH            = 0b01000000;
const uint8_t SUBCHANNEL_FLAG_IS_UEP            = 0b00100000;
const uint8_t SUBCHANNEL_FLAG_UEP_PROT_INDEX    = 0b00010000;
const uint8_t SUBCHANNEL_FLAG_EEP_PROT_LEVEL    = 0b00001000;
const uint8_t SUBCHANNEL_FLAG_EEP_TYPE          = 0b00000100;
const uint8_t SUBCHANNEL_FLAG_FEC_SCHEME        = 0b00000010;
const uint8_t SUBCHANNEL_FLAG_REQUIRED_UEP      = 0b11110000;
const uint8_t SUBCHANNEL_FLAG_REQUIRED_EEP      = 0b11101100;

UpdateResult SubchannelUpdater::SetStartAddress(const subchannel_addr_t start_address) {
    return UpdateField(GetData().start_address, start_address, SUBCHANNEL_FLAG_START_ADDRESS);
}

UpdateResult SubchannelUpdater::SetLength(const subchannel_size_t length) {
    return UpdateField(GetData().length, length, SUBCHANNEL_FLAG_LENGTH); 
}

UpdateResult SubchannelUpdater::SetIsUEP(const bool is_uep) {
    return UpdateField(GetData().is_uep, is_uep, SUBCHANNEL_FLAG_IS_UEP);
}

UpdateResult SubchannelUpdater::SetUEPProtIndex(const uep_protection_index_t uep_prot_index) {
    const auto res = SetIsUEP(true);
    if (res == UpdateResult::CONFLICT) {
        return UpdateResult::CONFLICT;
    }
    return UpdateField(GetData().uep_prot_index, uep_prot_index, SUBCHANNEL_FLAG_UEP_PROT_INDEX);
}

UpdateResult SubchannelUpdater::SetEEPProtLevel(const eep_protection_level_t eep_prot_level) {
    const auto res = SetIsUEP(false);
    if (res == UpdateResult::CONFLICT) {
        return UpdateResult::CONFLICT;
    }
    return UpdateField(GetData().eep_prot_level, eep_prot_level, SUBCHANNEL_FLAG_EEP_PROT_LEVEL);
}

UpdateResult SubchannelUpdater::SetEEPType(const EEP_Type eep_type) {
    const auto res = SetIsUEP(false);
    if (res == UpdateResult::CONFLICT) {
        return UpdateResult::CONFLICT;
    }
    return UpdateField(GetData().eep_type, eep_type, SUBCHANNEL_FLAG_EEP_TYPE);
}

UpdateResult SubchannelUpdater::SetFECScheme(const FEC_Scheme fec_scheme) {
    return UpdateField(GetData().fec_scheme, fec_scheme, SUBCHANNEL_FLAG_FEC_SCHEME);
}

bool SubchannelUpdater::IsComplete() {
    const bool eep_complete = (m_dirty_field & SUBCHANNEL_FLAG_REQUIRED_EEP) == SUBCHANNEL_FLAG_REQUIRED_EEP;
    const bool uep_complete = (m_dirty_field & SUBCHANNEL_FLAG_REQUIRED_UEP) == SUBCHANNEL_FLAG_REQUIRED_UEP;
    const bool is_complete = GetData().is_uep ? uep_complete : eep_complete;
    GetData().is_complete = is_complete;
    return is_complete;
}

// link service form
const uint8_t LINK_FLAG_ACTIVE          = 0b10000000;
const uint8_t LINK_FLAG_HARD            = 0b01000000;
const uint8_t LINK_FLAG_INTERNATIONAL   = 0b00100000;
const uint8_t LINK_FLAG_SERVICE_ID      = 0b00010000;
const uint8_t LINK_FLAG_REQUIRED        = 0b00010000;

UpdateResult LinkServiceUpdater::SetIsActiveLink(const bool is_active_link) {
    return UpdateField(GetData().is_active_link, is_active_link, LINK_FLAG_ACTIVE);
}

UpdateResult LinkServiceUpdater::SetIsHardLink(const bool is_hard_link) {
    return UpdateField(GetData().is_hard_link, is_hard_link, LINK_FLAG_HARD);
}

UpdateResult LinkServiceUpdater::SetIsInternational(const bool is_international) {
    return UpdateField(GetData().is_international, is_international, LINK_FLAG_INTERNATIONAL);
}

UpdateResult LinkServiceUpdater::SetServiceId(const ServiceId service_id) {
    return UpdateField(GetData().service_id, service_id, LINK_FLAG_SERVICE_ID);
}

bool LinkServiceUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & LINK_FLAG_REQUIRED) == LINK_FLAG_REQUIRED);
}

// fm service form
const uint8_t FM_FLAG_LSN       = 0b10000000;
const uint8_t FM_FLAG_TIME_COMP = 0b01000000;
const uint8_t FM_FLAG_FREQ      = 0b00100000;
const uint8_t FM_FLAG_REQUIRED  = 0b10100000;

UpdateResult FM_ServiceUpdater::SetLinkageSetNumber(const lsn_t linkage_set_number) {
    return UpdateField(GetData().linkage_set_number, linkage_set_number, FM_FLAG_LSN);
}

UpdateResult FM_ServiceUpdater::SetIsTimeCompensated(const bool is_time_compensated) {
    return UpdateField(GetData().is_time_compensated, is_time_compensated, FM_FLAG_TIME_COMP);
}

UpdateResult FM_ServiceUpdater::AddFrequency(const freq_t frequency) {
    if (!insert_if_unique(GetData().frequencies, frequency)) return UpdateResult::NO_CHANGE;
    m_dirty_field |= FM_FLAG_FREQ;
    OnComplete();
    OnUpdate();
    return UpdateResult::SUCCESS;
}

bool FM_ServiceUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & FM_FLAG_REQUIRED) == FM_FLAG_REQUIRED);
}

// drm service form
const uint8_t DRM_FLAG_LSN       = 0b10000000;
const uint8_t DRM_FLAG_TIME_COMP = 0b01000000;
const uint8_t DRM_FLAG_FREQ      = 0b00100000;
const uint8_t DRM_FLAG_REQUIRED  = 0b10100000;

UpdateResult DRM_ServiceUpdater::SetLinkageSetNumber(const lsn_t linkage_set_number) {
    return UpdateField(GetData().linkage_set_number, linkage_set_number, DRM_FLAG_LSN);
}

UpdateResult DRM_ServiceUpdater::SetIsTimeCompensated(const bool is_time_compensated) {
    return UpdateField(GetData().is_time_compensated, is_time_compensated, DRM_FLAG_TIME_COMP);
}

UpdateResult DRM_ServiceUpdater::AddFrequency(const freq_t frequency) {
    if (!insert_if_unique(GetData().frequencies, frequency)) return UpdateResult::NO_CHANGE;
    m_dirty_field |= DRM_FLAG_FREQ;
    OnComplete();
    OnUpdate();
    return UpdateResult::SUCCESS;
}

bool DRM_ServiceUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & DRM_FLAG_REQUIRED) == DRM_FLAG_REQUIRED);
}

// amss service form
const uint8_t AMSS_FLAG_TIME_COMP = 0b10000000;
const uint8_t AMSS_FLAG_FREQ      = 0b01000000;
const uint8_t AMSS_FLAG_REQUIRED  = 0b01000000;

UpdateResult AMSS_ServiceUpdater::SetIsTimeCompensated(const bool is_time_compensated) {
    return UpdateField(GetData().is_time_compensated, is_time_compensated, AMSS_FLAG_TIME_COMP);
}

UpdateResult AMSS_ServiceUpdater::AddFrequency(const freq_t frequency) {
    if (!insert_if_unique(GetData().frequencies, frequency)) return UpdateResult::NO_CHANGE;
    m_dirty_field |= AMSS_FLAG_FREQ;
    OnComplete();
    OnUpdate();
    return UpdateResult::SUCCESS;
}

bool AMSS_ServiceUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & AMSS_FLAG_REQUIRED) == AMSS_FLAG_REQUIRED);
}

// other ensemble form
const uint8_t OE_FLAG_CONT_OUT   = 0b01000000;
const uint8_t OE_FLAG_GEO_ADJ    = 0b00100000;
const uint8_t OE_FLAG_MODE_I     = 0b00010000;
const uint8_t OE_FLAG_FREQ       = 0b00001000;
const uint8_t OE_FLAG_REQUIRED   = 0b00001000;

UpdateResult OtherEnsembleUpdater::SetIsContinuousOutput(const bool is_continuous_output) {
    return UpdateField(GetData().is_continuous_output, is_continuous_output, OE_FLAG_CONT_OUT);
}

UpdateResult OtherEnsembleUpdater::SetIsGeographicallyAdjacent(const bool is_geographically_adjacent) {
    return UpdateField(GetData().is_geographically_adjacent, is_geographically_adjacent, OE_FLAG_GEO_ADJ);
}

UpdateResult OtherEnsembleUpdater::SetIsTransmissionModeI(const bool is_transmission_mode_I) {
    return UpdateField(GetData().is_transmission_mode_I, is_transmission_mode_I, OE_FLAG_MODE_I);
}

UpdateResult OtherEnsembleUpdater::SetFrequency(const freq_t frequency) {
    return UpdateField(GetData().frequency, frequency, OE_FLAG_FREQ);
}

bool OtherEnsembleUpdater::IsComplete() {
    return GetData().is_complete = ((m_dirty_field & OE_FLAG_REQUIRED) == OE_FLAG_REQUIRED);
}

// updater parent
DAB_Database_Updater::DAB_Database_Updater() {
    m_db = std::make_unique<DAB_Database>();
    m_stats = std::make_unique<DatabaseUpdaterGlobalStatistics>();
    m_ensemble_updater = std::make_unique<EnsembleUpdater>(*(m_db.get()), *(m_stats.get()));
}

ServiceUpdater& DAB_Database_Updater::GetServiceUpdater(const ServiceId service_id) {
    const auto uuid = service_id.get_unique_identifier();
    auto& updater = find_or_insert_updater(
        m_db->services, m_service_updaters,
        [uuid](const auto& e) {
            return e.id.get_unique_identifier() == uuid;
        },
        service_id
    );
    // Upgrade 16bit ids to 24bit id 
    // 24bit id still uses 16bits for unique identifier but has 8bit extended country code
    if (service_id.type == ServiceIdType::BITS24) {
        updater.GetData().id = service_id;
    }
    return updater;
}

ServiceComponentUpdater& DAB_Database_Updater::GetServiceComponentUpdater_Service(
    const ServiceId service_id, const uint16_t unique_id) 
{
    const auto service_uuid = service_id.get_unique_identifier();
    return find_or_insert_updater(
        m_db->service_components, m_service_component_updaters,
        [service_uuid, unique_id](const auto& e) { 
            return (e.service_id.get_unique_identifier() == service_uuid) && (e.unique_id == unique_id); 
        },
        service_id, unique_id
    );
}

SubchannelUpdater& DAB_Database_Updater::GetSubchannelUpdater(const subchannel_id_t subchannel_id) {
    return find_or_insert_updater(
        m_db->subchannels, m_subchannel_updaters,
        [subchannel_id](const auto& e) {
            return e.id == subchannel_id;
        },
        subchannel_id
    );
}

LinkServiceUpdater& DAB_Database_Updater::GetLinkServiceUpdater(const lsn_t link_service_number) {
    return find_or_insert_updater(
        m_db->link_services, m_link_service_updaters,
        [link_service_number](const auto& e) {
            return e.id == link_service_number;
        },
        link_service_number
    );
}

FM_ServiceUpdater& DAB_Database_Updater::GetFMServiceUpdater(const fm_id_t RDS_PI_code) {
    return find_or_insert_updater(
        m_db->fm_services, m_fm_service_updaters,
        [RDS_PI_code](const auto& e) {
            return e.RDS_PI_code == RDS_PI_code;
        },
        RDS_PI_code
    );
}

DRM_ServiceUpdater& DAB_Database_Updater::GetDRMServiceUpdater(const drm_id_t drm_code) {
    return find_or_insert_updater(
        m_db->drm_services, m_drm_service_updaters,
        [drm_code](const auto& e) {
            return e.drm_code == drm_code;
        },
        drm_code
    );
}

AMSS_ServiceUpdater& DAB_Database_Updater::GetAMSS_ServiceUpdater(const amss_id_t amss_code) {
    return find_or_insert_updater(
        m_db->amss_services, m_amss_service_updaters,
        [amss_code](const auto& e) {
            return e.amss_code == amss_code;
        },
        amss_code
    );
}

OtherEnsembleUpdater& DAB_Database_Updater::GetOtherEnsemble(const EnsembleId ensemble_id) {
    const auto ensemble_uuid = ensemble_id.get_unique_identifier();
    return find_or_insert_updater(
        m_db->other_ensembles, m_other_ensemble_updaters,
        [ensemble_uuid](const auto& e) {
            return e.id.get_unique_identifier() == ensemble_uuid;
        },
        ensemble_id
    );
}

ServiceComponentUpdater* DAB_Database_Updater::GetServiceComponentUpdater_ComponentID(
    const ServiceId service_id, const service_component_id_t component_id)
{
    const auto service_uuid = service_id.get_unique_identifier();
    return find_updater(
        m_db->service_components, m_service_component_updaters,
        [service_uuid, component_id](const auto& e) {
            return (e.service_id.get_unique_identifier() == service_uuid) && (e.component_id == component_id);
        }
    );
}

ServiceComponentUpdater* DAB_Database_Updater::GetServiceComponentUpdater_GlobalID(
    const service_component_global_id_t global_id) 
{
    return find_updater(
        m_db->service_components, m_service_component_updaters,
        [global_id](const auto& e) {
            return e.global_id == global_id;
        }
    );
}

ServiceComponentUpdater* DAB_Database_Updater::GetServiceComponentUpdater_Subchannel(
    const ServiceId service_id, const subchannel_id_t subchannel_id) 
{
    const auto service_uuid = service_id.get_unique_identifier();
    return find_updater(
        m_db->service_components, m_service_component_updaters,
        [service_uuid, subchannel_id](const auto& e) {
            return (e.service_id.get_unique_identifier() == service_uuid) && (e.subchannel_id == subchannel_id);
        }
    );
}