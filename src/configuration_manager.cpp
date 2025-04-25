#include "configuration_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <json/json.h>

namespace mfp {

// Static instance for global access
static ConfigurationManager s_config_instance;

// ConfigProfile implementation
ConfigProfile::ConfigProfile(ProfileType type, const std::string& name)
    : m_type(type), m_name(name) {
}

ProfileType ConfigProfile::getType() const {
    return m_type;
}

const std::string& ConfigProfile::getName() const {
    return m_name;
}

void ConfigProfile::setParam(const std::string& name, const ConfigParam& param) {
    m_params[name] = param;
}

const ConfigParam* ConfigProfile::getParam(const std::string& name) const {
    auto it = m_params.find(name);
    if (it != m_params.end()) {
        return &it->second;
    }
    return nullptr;
}

void ConfigProfile::apply(ResourceManager& resource_manager) const {
    // Apply execution strategy
    auto strategy_param = getParam("execution_strategy");
    if (strategy_param && strategy_param->type == ParamType::ENUM) {
        if (strategy_param->string_value == "auto") {
            resource_manager.setExecutionStrategy(ExecutionStrategy::AUTO);
        } else if (strategy_param->string_value == "cpu_only") {
            resource_manager.setExecutionStrategy(ExecutionStrategy::CPU_ONLY);
        } else if (strategy_param->string_value == "cuda_gpu") {
            resource_manager.setExecutionStrategy(ExecutionStrategy::CUDA_GPU);
        } else if (strategy_param->string_value == "metal_gpu") {
            resource_manager.setExecutionStrategy(ExecutionStrategy::METAL_GPU);
        } else if (strategy_param->string_value == "hybrid") {
            resource_manager.setExecutionStrategy(ExecutionStrategy::HYBRID);
        }
    }
    
    // Apply allocation mode
    auto mode_param = getParam("allocation_mode");
    if (mode_param && mode_param->type == ParamType::ENUM) {
        if (mode_param->string_value == "auto") {
            resource_manager.setAllocationMode(AllocationMode::AUTO);
        } else if (mode_param->string_value == "performance") {
            resource_manager.setAllocationMode(AllocationMode::PERFORMANCE);
        } else if (mode_param->string_value == "memory") {
            resource_manager.setAllocationMode(AllocationMode::MEMORY);
        } else if (mode_param->string_value == "balanced") {
            resource_manager.setAllocationMode(AllocationMode::BALANCED);
        }
    }
    
    // Other parameters would be applied here
    // For example, thread count, block size, etc.
}

ConfigProfile ConfigProfile::createFromResourceManager(const ResourceManager& resource_manager, const std::string& name) {
    ConfigProfile profile(ProfileType::CUSTOM, name);
    
    // Set execution strategy
    std::string strategy_value;
    switch (resource_manager.getExecutionStrategy()) {
        case ExecutionStrategy::AUTO: strategy_value = "auto"; break;
        case ExecutionStrategy::CPU_ONLY: strategy_value = "cpu_only"; break;
        case ExecutionStrategy::CUDA_GPU: strategy_value = "cuda_gpu"; break;
        case ExecutionStrategy::METAL_GPU: strategy_value = "metal_gpu"; break;
        case ExecutionStrategy::HYBRID: strategy_value = "hybrid"; break;
    }
    profile.setParam("execution_strategy", ConfigParam("execution_strategy", strategy_value, ParamType::ENUM));
    
    // Set allocation mode
    std::string mode_value;
    switch (resource_manager.getAllocationMode()) {
        case AllocationMode::AUTO: mode_value = "auto"; break;
        case AllocationMode::PERFORMANCE: mode_value = "performance"; break;
        case AllocationMode::MEMORY: mode_value = "memory"; break;
        case AllocationMode::BALANCED: mode_value = "balanced"; break;
    }
    profile.setParam("allocation_mode", ConfigParam("allocation_mode", mode_value, ParamType::ENUM));
    
    // Other parameters would be captured here
    
    return profile;
}

ConfigProfile ConfigProfile::createDefaultProfile(const ResourceManager& resource_manager) {
    // Determine profile type based on hardware
    ProfileType profile_type = ProfileType::MID_RANGE;
    
    const CPUInfo& cpu_info = resource_manager.getCPUInfo();
    const MemoryInfo& memory_info = resource_manager.getMemoryInfo();
    const std::vector<GPUInfo>& gpus = resource_manager.getGPUs();
    
    // Classify hardware
    if (cpu_info.physical_cores >= 16 && memory_info.total_physical_memory >= 128ULL * 1024 * 1024 * 1024) {
        // Server class: 16+ cores, 128+ GB RAM
        profile_type = ProfileType::SERVER;
    } else if (cpu_info.physical_cores >= 8 && memory_info.total_physical_memory >= 32ULL * 1024 * 1024 * 1024 && !gpus.empty()) {
        // Workstation class: 8+ cores, 32+ GB RAM, dedicated GPU
        profile_type = ProfileType::WORKSTATION;
    } else if (cpu_info.physical_cores >= 6 && memory_info.total_physical_memory >= 16ULL * 1024 * 1024 * 1024) {
        // High-end class: 6+ cores, 16+ GB RAM
        profile_type = ProfileType::HIGH_END;
    } else if (cpu_info.physical_cores >= 4 && memory_info.total_physical_memory >= 8ULL * 1024 * 1024 * 1024) {
        // Mid-range class: 4+ cores, 8+ GB RAM
        profile_type = ProfileType::MID_RANGE;
    } else {
        // Low-end class: <4 cores or <8 GB RAM
        profile_type = ProfileType::LOW_END;
    }
    
    // Create profile with appropriate name
    std::string profile_name;
    switch (profile_type) {
        case ProfileType::SERVER: profile_name = "Server"; break;
        case ProfileType::WORKSTATION: profile_name = "Workstation"; break;
        case ProfileType::HIGH_END: profile_name = "High-End"; break;
        case ProfileType::MID_RANGE: profile_name = "Mid-Range"; break;
        case ProfileType::LOW_END: profile_name = "Low-End"; break;
        default: profile_name = "Custom"; break;
    }
    
    ConfigProfile profile(profile_type, profile_name);
    
    // Set default parameters based on profile type
    switch (profile_type) {
        case ProfileType::SERVER:
            // Server profile: Performance-oriented
            profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
            profile.setParam("allocation_mode", ConfigParam("allocation_mode", "performance", ParamType::ENUM));
            break;
            
        case ProfileType::WORKSTATION:
            // Workstation profile: GPU-accelerated
            profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
            profile.setParam("allocation_mode", ConfigParam("allocation_mode", "balanced", ParamType::ENUM));
            break;
            
        case ProfileType::HIGH_END:
            // High-end profile: Balanced
            profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
            profile.setParam("allocation_mode", ConfigParam("allocation_mode", "balanced", ParamType::ENUM));
            break;
            
        case ProfileType::MID_RANGE:
            // Mid-range profile: Balanced
            profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
            profile.setParam("allocation_mode", ConfigParam("allocation_mode", "balanced", ParamType::ENUM));
            break;
            
        case ProfileType::LOW_END:
            // Low-end profile: Memory-conservative
            profile.setParam("execution_strategy", ConfigParam("execution_strategy", "cpu_only", ParamType::ENUM));
            profile.setParam("allocation_mode", ConfigParam("allocation_mode", "memory", ParamType::ENUM));
            break;
            
        default:
            // Custom profile: Auto settings
            profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
            profile.setParam("allocation_mode", ConfigParam("allocation_mode", "auto", ParamType::ENUM));
            break;
    }
    
    return profile;
}

// ConfigurationManager implementation
ConfigurationManager::ConfigurationManager()
    : m_resource_manager(nullptr), m_current_profile(ProfileType::CUSTOM, "Default") {
}

ConfigurationManager::~ConfigurationManager() {
    // Cleanup happens in member destructors
}

bool ConfigurationManager::initialize(ResourceManager& resource_manager) {
    m_resource_manager = &resource_manager;
    
    // Create default profiles
    createDefaultProfiles();
    
    // Create and set default profile based on hardware
    m_current_profile = ConfigProfile::createDefaultProfile(resource_manager);
    
    // Apply current profile
    m_current_profile.apply(resource_manager);
    
    return true;
}

const ConfigProfile& ConfigurationManager::getCurrentProfile() const {
    return m_current_profile;
}

void ConfigurationManager::setCurrentProfile(const ConfigProfile& profile) {
    m_current_profile = profile;
    
    // Apply profile to resource manager
    if (m_resource_manager) {
        m_current_profile.apply(*m_resource_manager);
    }
}

const std::map<std::string, ConfigProfile>& ConfigurationManager::getProfiles() const {
    return m_profiles;
}

void ConfigurationManager::addProfile(const ConfigProfile& profile) {
    m_profiles[profile.getName()] = profile;
}

void ConfigurationManager::removeProfile(const std::string& name) {
    m_profiles.erase(name);
}

bool ConfigurationManager::saveToFile(const std::string& filename) const {
    Json::Value root;
    
    // Save current profile
    Json::Value current_profile;
    current_profile["name"] = m_current_profile.getName();
    current_profile["type"] = static_cast<int>(m_current_profile.getType());
    
    Json::Value current_params;
    for (const auto& param_pair : m_profiles.at(m_current_profile.getName()).m_params) {
        const std::string& param_name = param_pair.first;
        const ConfigParam& param = param_pair.second;
        
        Json::Value param_value;
        param_value["name"] = param.name;
        param_value["type"] = static_cast<int>(param.type);
        
        switch (param.type) {
            case ParamType::INT:
                param_value["value"] = param.int_value;
                break;
                
            case ParamType::DOUBLE:
                param_value["value"] = param.double_value;
                break;
                
            case ParamType::BOOL:
                param_value["value"] = param.bool_value;
                break;
                
            case ParamType::STRING:
            case ParamType::ENUM:
                param_value["value"] = param.string_value;
                break;
        }
        
        current_params[param_name] = param_value;
    }
    
    current_profile["params"] = current_params;
    root["current_profile"] = current_profile;
    
    // Save profiles
    Json::Value profiles;
    for (const auto& profile_pair : m_profiles) {
        const std::string& profile_name = profile_pair.first;
        const ConfigProfile& profile = profile_pair.second;
        
        Json::Value profile_value;
        profile_value["name"] = profile.getName();
        profile_value["type"] = static_cast<int>(profile.getType());
        
        Json::Value params;
        for (const auto& param_pair : profile.m_params) {
            const std::string& param_name = param_pair.first;
            const ConfigParam& param = param_pair.second;
            
            Json::Value param_value;
            param_value["name"] = param.name;
            param_value["type"] = static_cast<int>(param.type);
            
            switch (param.type) {
                case ParamType::INT:
                    param_value["value"] = param.int_value;
                    break;
                    
                case ParamType::DOUBLE:
                    param_value["value"] = param.double_value;
                    break;
                    
                case ParamType::BOOL:
                    param_value["value"] = param.bool_value;
                    break;
                    
                case ParamType::STRING:
                case ParamType::ENUM:
                    param_value["value"] = param.string_value;
                    break;
            }
            
            params[param_name] = param_value;
        }
        
        profile_value["params"] = params;
        profiles[profile_name] = profile_value;
    }
    
    root["profiles"] = profiles;
    
    // Write to file
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << root;
    return true;
}

bool ConfigurationManager::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    Json::Value root;
    file >> root;
    
    // Load profiles
    const Json::Value& profiles = root["profiles"];
    for (const auto& profile_name : profiles.getMemberNames()) {
        const Json::Value& profile_value = profiles[profile_name];
        
        ProfileType profile_type = static_cast<ProfileType>(profile_value["type"].asInt());
        ConfigProfile profile(profile_type, profile_name);
        
        const Json::Value& params = profile_value["params"];
        for (const auto& param_name : params.getMemberNames()) {
            const Json::Value& param_value = params[param_name];
            
            ParamType param_type = static_cast<ParamType>(param_value["type"].asInt());
            
            switch (param_type) {
                case ParamType::INT:
                    profile.setParam(param_name, ConfigParam(param_name, param_value["value"].asInt()));
                    break;
                    
                case ParamType::DOUBLE:
                    profile.setParam(param_name, ConfigParam(param_name, param_value["value"].asDouble()));
                    break;
                    
                case ParamType::BOOL:
                    profile.setParam(param_name, ConfigParam(param_name, param_value["value"].asBool()));
                    break;
                    
                case ParamType::STRING:
                    profile.setParam(param_name, ConfigParam(param_name, param_value["value"].asString()));
                    break;
                    
                case ParamType::ENUM:
                    profile.setParam(param_name, ConfigParam(param_name, param_value["value"].asString(), ParamType::ENUM));
                    break;
            }
        }
        
        addProfile(profile);
    }
    
    // Load current profile
    const Json::Value& current_profile = root["current_profile"];
    std::string current_profile_name = current_profile["name"].asString();
    
    if (m_profiles.find(current_profile_name) != m_profiles.end()) {
        setCurrentProfile(m_profiles[current_profile_name]);
    }
    
    return true;
}

std::string ConfigurationManager::getConfigurationSummary() const {
    std::stringstream ss;
    
    ss << "Configuration Summary:" << std::endl;
    ss << "======================" << std::endl;
    
    // Current profile
    ss << "Current Profile: " << m_current_profile.getName() << std::endl;
    ss << "  Type: ";
    switch (m_current_profile.getType()) {
        case ProfileType::LOW_END: ss << "Low-End"; break;
        case ProfileType::MID_RANGE: ss << "Mid-Range"; break;
        case ProfileType::HIGH_END: ss << "High-End"; break;
        case ProfileType::SERVER: ss << "Server"; break;
        case ProfileType::WORKSTATION: ss << "Workstation"; break;
        case ProfileType::CUSTOM: ss << "Custom"; break;
    }
    ss << std::endl;
    
    // Profile parameters
    ss << "  Parameters:" << std::endl;
    for (const auto& param_pair : m_profiles.at(m_current_profile.getName()).m_params) {
        const std::string& param_name = param_pair.first;
        const ConfigParam& param = param_pair.second;
        
        ss << "    " << param.name << ": ";
        
        switch (param.type) {
            case ParamType::INT:
                ss << param.int_value;
                break;
                
            case ParamType::DOUBLE:
                ss << param.double_value;
                break;
                
            case ParamType::BOOL:
                ss << (param.bool_value ? "true" : "false");
                break;
                
            case ParamType::STRING:
            case ParamType::ENUM:
                ss << param.string_value;
                break;
        }
        
        ss << std::endl;
    }
    
    // Available profiles
    ss << "Available Profiles:" << std::endl;
    for (const auto& profile_pair : m_profiles) {
        const std::string& profile_name = profile_pair.first;
        const ConfigProfile& profile = profile_pair.second;
        
        ss << "  " << profile_name << " (";
        switch (profile.getType()) {
            case ProfileType::LOW_END: ss << "Low-End"; break;
            case ProfileType::MID_RANGE: ss << "Mid-Range"; break;
            case ProfileType::HIGH_END: ss << "High-End"; break;
            case ProfileType::SERVER: ss << "Server"; break;
            case ProfileType::WORKSTATION: ss << "Workstation"; break;
            case ProfileType::CUSTOM: ss << "Custom"; break;
        }
        ss << ")" << std::endl;
    }
    
    return ss.str();
}

void ConfigurationManager::createDefaultProfiles() {
    // Create profiles for different hardware classes
    if (!m_resource_manager) {
        return;
    }
    
    // Low-end profile
    {
        ConfigProfile profile(ProfileType::LOW_END, "Low-End");
        profile.setParam("execution_strategy", ConfigParam("execution_strategy", "cpu_only", ParamType::ENUM));
        profile.setParam("allocation_mode", ConfigParam("allocation_mode", "memory", ParamType::ENUM));
        addProfile(profile);
    }
    
    // Mid-range profile
    {
        ConfigProfile profile(ProfileType::MID_RANGE, "Mid-Range");
        profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
        profile.setParam("allocation_mode", ConfigParam("allocation_mode", "balanced", ParamType::ENUM));
        addProfile(profile);
    }
    
    // High-end profile
    {
        ConfigProfile profile(ProfileType::HIGH_END, "High-End");
        profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
        profile.setParam("allocation_mode", ConfigParam("allocation_mode", "balanced", ParamType::ENUM));
        addProfile(profile);
    }
    
    // Server profile
    {
        ConfigProfile profile(ProfileType::SERVER, "Server");
        profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
        profile.setParam("allocation_mode", ConfigParam("allocation_mode", "performance", ParamType::ENUM));
        addProfile(profile);
    }
    
    // Workstation profile
    {
        ConfigProfile profile(ProfileType::WORKSTATION, "Workstation");
        profile.setParam("execution_strategy", ConfigParam("execution_strategy", "auto", ParamType::ENUM));
        profile.setParam("allocation_mode", ConfigParam("allocation_mode", "balanced", ParamType::ENUM));
        addProfile(profile);
    }
    
    // Add current hardware profile
    ConfigProfile current_profile = ConfigProfile::createDefaultProfile(*m_resource_manager);
    addProfile(current_profile);
}

// Global configuration manager instance
ConfigurationManager& getConfigurationManager() {
    return s_config_instance;
}

} // namespace mfp
