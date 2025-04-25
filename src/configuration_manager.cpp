#include "configuration_manager.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <iomanip>
#include <cmath>
#include <iostream>

namespace mfp {
namespace config {

//=============================================================================
// ConfigProfile Implementation
//=============================================================================

ConfigProfile::ConfigProfile(const std::string& name, HardwareClass hardware_class)
    : name_(name),
      hardware_class_(hardware_class) {
}

std::string ConfigProfile::getName() const {
    return name_;
}

HardwareClass ConfigProfile::getHardwareClass() const {
    return hardware_class_;
}

void ConfigProfile::setParameter(const std::string& name, const std::string& value, bool auto_configured) {
    // Check if parameter exists
    auto it = parameters_.find(name);
    if (it != parameters_.end()) {
        // Update existing parameter
        it->second.current_value = value;
        it->second.auto_configured = auto_configured;
    } else {
        // Create new parameter
        ConfigParameter param;
        param.name = name;
        param.current_value = value;
        param.auto_configured = auto_configured;
        parameters_[name] = param;
    }
}

std::string ConfigProfile::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) {
        return it->second.current_value;
    }
    return "";
}

bool ConfigProfile::hasParameter(const std::string& name) const {
    return parameters_.find(name) != parameters_.end();
}

const std::map<std::string, ConfigParameter>& ConfigProfile::getParameters() const {
    return parameters_;
}

bool ConfigProfile::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Clear existing parameters
    parameters_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse parameter line (name=value)
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Set parameter
            setParameter(name, value, false);
        }
    }
    
    file.close();
    return true;
}

bool ConfigProfile::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "# MFP Configuration Profile: " << name_ << std::endl;
    file << "# Hardware Class: " << static_cast<int>(hardware_class_) << std::endl;
    file << std::endl;
    
    // Write parameters
    for (const auto& param : parameters_) {
        file << param.first << " = " << param.second.current_value << std::endl;
    }
    
    file.close();
    return true;
}

std::string ConfigProfile::toString() const {
    std::stringstream ss;
    ss << "Profile: " << name_ << std::endl;
    ss << "Hardware Class: ";
    
    switch (hardware_class_) {
        case HardwareClass::LOW_END:
            ss << "LOW_END";
            break;
        case HardwareClass::MID_RANGE:
            ss << "MID_RANGE";
            break;
        case HardwareClass::HIGH_END:
            ss << "HIGH_END";
            break;
        case HardwareClass::SERVER:
            ss << "SERVER";
            break;
        case HardwareClass::WORKSTATION:
            ss << "WORKSTATION";
            break;
        case HardwareClass::CUSTOM:
            ss << "CUSTOM";
            break;
    }
    ss << std::endl;
    
    ss << "Parameters:" << std::endl;
    for (const auto& param : parameters_) {
        ss << "  " << param.first << " = " << param.second.current_value;
        if (param.second.auto_configured) {
            ss << " (auto-configured)";
        }
        ss << std::endl;
    }
    
    return ss.str();
}

//=============================================================================
// ConfigurationManager Implementation
//=============================================================================

ConfigurationManager::ConfigurationManager()
    : resource_manager_(nullptr),
      current_profile_(nullptr) {
}

ConfigurationManager::~ConfigurationManager() {
    // Clean up resources
}

bool ConfigurationManager::initialize(resource::ResourceManager* resource_manager) {
    if (!resource_manager) {
        return false;
    }
    
    resource_manager_ = resource_manager;
    
    // Register default parameters
    registerDefaultParameters();
    
    // Create default profiles
    createDefaultProfiles();
    
    // Set default profile
    current_profile_ = profiles_["default"].get();
    
    return true;
}

bool ConfigurationManager::autoConfigureForHardware() {
    if (!resource_manager_) {
        return false;
    }
    
    // Classify hardware
    HardwareClass hardware_class = classifyHardware();
    
    // Create or select appropriate profile
    std::string profile_name;
    
    switch (hardware_class) {
        case HardwareClass::LOW_END:
            profile_name = "low_end";
            break;
        case HardwareClass::MID_RANGE:
            profile_name = "mid_range";
            break;
        case HardwareClass::HIGH_END:
            profile_name = "high_end";
            break;
        case HardwareClass::SERVER:
            profile_name = "server";
            break;
        case HardwareClass::WORKSTATION:
            profile_name = "workstation";
            break;
        default:
            profile_name = "default";
            break;
    }
    
    // Set current profile
    auto it = profiles_.find(profile_name);
    if (it != profiles_.end()) {
        current_profile_ = it->second.get();
    } else {
        // Create new profile if it doesn't exist
        current_profile_ = createProfile(profile_name, hardware_class);
    }
    
    // Configure profile based on hardware class
    switch (hardware_class) {
        case HardwareClass::LOW_END:
            configureForLowEndHardware(current_profile_);
            break;
        case HardwareClass::MID_RANGE:
            configureForMidRangeHardware(current_profile_);
            break;
        case HardwareClass::HIGH_END:
            configureForHighEndHardware(current_profile_);
            break;
        case HardwareClass::SERVER:
            configureForServerHardware(current_profile_);
            break;
        case HardwareClass::WORKSTATION:
            configureForWorkstationHardware(current_profile_);
            break;
        default:
            // Use default configuration
            break;
    }
    
    // Apply configuration to resource manager
    applyConfiguration();
    
    return true;
}

bool ConfigurationManager::loadConfiguration(const std::string& filename) {
    // Create a temporary profile
    auto temp_profile = std::make_unique<ConfigProfile>("temp", HardwareClass::CUSTOM);
    
    // Load profile from file
    if (!temp_profile->loadFromFile(filename)) {
        return false;
    }
    
    // Check if profile has a name parameter
    std::string profile_name = temp_profile->getParameter("profile_name");
    if (profile_name.empty()) {
        profile_name = "loaded_profile";
    }
    
    // Check if profile has a hardware class parameter
    std::string hardware_class_str = temp_profile->getParameter("hardware_class");
    HardwareClass hardware_class = HardwareClass::CUSTOM;
    
    if (!hardware_class_str.empty()) {
        try {
            int hardware_class_int = std::stoi(hardware_class_str);
            hardware_class = static_cast<HardwareClass>(hardware_class_int);
        } catch (...) {
            // Ignore conversion errors
        }
    }
    
    // Create new profile
    auto profile = createProfile(profile_name, hardware_class);
    
    // Copy parameters from temporary profile
    for (const auto& param : temp_profile->getParameters()) {
        profile->setParameter(param.first, param.second.current_value, false);
    }
    
    // Set as current profile
    current_profile_ = profile;
    
    // Apply configuration to resource manager
    applyConfiguration();
    
    return true;
}

bool ConfigurationManager::saveConfiguration(const std::string& filename) const {
    if (!current_profile_) {
        return false;
    }
    
    return current_profile_->saveToFile(filename);
}

ConfigProfile* ConfigurationManager::getCurrentProfile() {
    return current_profile_;
}

void ConfigurationManager::setCurrentProfile(const std::string& profile_name) {
    auto it = profiles_.find(profile_name);
    if (it != profiles_.end()) {
        current_profile_ = it->second.get();
        
        // Apply configuration to resource manager
        applyConfiguration();
    }
}

ConfigProfile* ConfigurationManager::createProfile(const std::string& name, HardwareClass hardware_class) {
    // Create new profile
    auto profile = std::make_unique<ConfigProfile>(name, hardware_class);
    
    // Add profile to map
    auto result = profile.get();
    profiles_[name] = std::move(profile);
    
    return result;
}

bool ConfigurationManager::deleteProfile(const std::string& name) {
    // Check if profile exists
    auto it = profiles_.find(name);
    if (it == profiles_.end()) {
        return false;
    }
    
    // Check if it's the current profile
    if (current_profile_ == it->second.get()) {
        // Set current profile to default
        auto default_it = profiles_.find("default");
        if (default_it != profiles_.end()) {
            current_profile_ = default_it->second.get();
        } else {
            current_profile_ = nullptr;
        }
    }
    
    // Remove profile
    profiles_.erase(it);
    
    return true;
}

ConfigProfile* ConfigurationManager::getProfile(const std::string& name) {
    auto it = profiles_.find(name);
    if (it != profiles_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const std::map<std::string, std::unique_ptr<ConfigProfile>>& ConfigurationManager::getProfiles() const {
    return profiles_;
}

void ConfigurationManager::registerParameter(const std::string& name, ParamType type, 
                                           const std::string& description, 
                                           const std::string& default_value, 
                                           const std::vector<std::string>& allowed_values) {
    // Create parameter definition
    ConfigParameter param;
    param.name = name;
    param.type = type;
    param.description = description;
    param.default_value = default_value;
    param.current_value = default_value;
    param.allowed_values = allowed_values;
    param.auto_configured = false;
    
    // Add to parameter definitions
    parameter_definitions_[name] = param;
}

std::string ConfigurationManager::getParameter(const std::string& name) const {
    if (!current_profile_) {
        // If no current profile, return default value from parameter definitions
        auto it = parameter_definitions_.find(name);
        if (it != parameter_definitions_.end()) {
            return it->second.default_value;
        }
        return "";
    }
    
    // Get parameter from current profile
    std::string value = current_profile_->getParameter(name);
    
    // If parameter not found in profile, return default value
    if (value.empty()) {
        auto it = parameter_definitions_.find(name);
        if (it != parameter_definitions_.end()) {
            return it->second.default_value;
        }
    }
    
    return value;
}

void ConfigurationManager::setParameter(const std::string& name, const std::string& value, bool auto_configured) {
    if (!current_profile_) {
        return;
    }
    
    // Check if parameter is defined
    auto it = parameter_definitions_.find(name);
    if (it == parameter_definitions_.end()) {
        // Parameter not defined, register it
        registerParameter(name, ParamType::STRING, "User-defined parameter", value);
    }
    
    // Set parameter in current profile
    current_profile_->setParameter(name, value, auto_configured);
}

int ConfigurationManager::getIntParameter(const std::string& name) const {
    std::string value = getParameter(name);
    if (value.empty()) {
        return 0;
    }
    
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

float ConfigurationManager::getFloatParameter(const std::string& name) const {
    std::string value = getParameter(name);
    if (value.empty()) {
        return 0.0f;
    }
    
    try {
        return std::stof(value);
    } catch (...) {
        return 0.0f;
    }
}

bool ConfigurationManager::getBoolParameter(const std::string& name) const {
    std::string value = getParameter(name);
    if (value.empty()) {
        return false;
    }
    
    // Convert to lowercase
    std::transform(value.begin(), value.end(), value.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    
    return value == "true" || value == "yes" || value == "1" || value == "on";
}

std::string ConfigurationManager::getParameterDescription(const std::string& name) const {
    auto it = parameter_definitions_.find(name);
    if (it != parameter_definitions_.end()) {
        return it->second.description;
    }
    return "";
}

ParamType ConfigurationManager::getParameterType(const std::string& name) const {
    auto it = parameter_definitions_.find(name);
    if (it != parameter_definitions_.end()) {
        return it->second.type;
    }
    return ParamType::STRING;
}

std::vector<std::string> ConfigurationManager::getAllowedValues(const std::string& name) const {
    auto it = parameter_definitions_.find(name);
    if (it != parameter_definitions_.end()) {
        return it->second.allowed_values;
    }
    return {};
}

bool ConfigurationManager::isAutoConfigured(const std::string& name) const {
    if (!current_profile_) {
        return false;
    }
    
    // Check if parameter exists in current profile
    auto params = current_profile_->getParameters();
    auto it = params.find(name);
    if (it != params.end()) {
        return it->second.auto_configured;
    }
    
    return false;
}

void ConfigurationManager::resetParameter(const std::string& name) {
    if (!current_profile_) {
        return;
    }
    
    // Get default value
    auto it = parameter_definitions_.find(name);
    if (it != parameter_definitions_.end()) {
        // Set parameter to default value
        current_profile_->setParameter(name, it->second.default_value, false);
    }
}

void ConfigurationManager::resetAllParameters() {
    if (!current_profile_) {
        return;
    }
    
    // Reset all parameters to default values
    for (const auto& param : parameter_definitions_) {
        current_profile_->setParameter(param.first, param.second.default_value, false);
    }
}

std::string ConfigurationManager::getConfigurationSummary() const {
    std::stringstream ss;
    
    ss << "MFP Configuration Summary" << std::endl;
    ss << "=========================" << std::endl;
    
    if (current_profile_) {
        ss << "Current Profile: " << current_profile_->getName() << std::endl;
        ss << "Hardware Class: ";
        
        switch (current_profile_->getHardwareClass()) {
            case HardwareClass::LOW_END:
                ss << "LOW_END";
                break;
            case HardwareClass::MID_RANGE:
                ss << "MID_RANGE";
                break;
            case HardwareClass::HIGH_END:
                ss << "HIGH_END";
                break;
            case HardwareClass::SERVER:
                ss << "SERVER";
                break;
            case HardwareClass::WORKSTATION:
                ss << "WORKSTATION";
                break;
            case HardwareClass::CUSTOM:
                ss << "CUSTOM";
                break;
        }
        ss << std::endl << std::endl;
        
        ss << "Parameters:" << std::endl;
        
        // Get all parameters from current profile
        auto params = current_profile_->getParameters();
        
        // Sort parameters by name
        std::vector<std::string> param_names;
        for (const auto& param : params) {
            param_names.push_back(param.first);
        }
        std::sort(param_names.begin(), param_names.end());
        
        // Print parameters
        for (const auto& name : param_names) {
            auto it = params.find(name);
            if (it != params.end()) {
                ss << "  " << name << " = " << it->second.current_value;
                
                // Add parameter type and description
                auto def_it = parameter_definitions_.find(name);
                if (def_it != parameter_definitions_.end()) {
                    ss << " (" << paramTypeToString(def_it->second.type) << ")";
                    
                    if (!def_it->second.description.empty()) {
                        ss << " - " << def_it->second.description;
                    }
                }
                
                if (it->second.auto_configured) {
                    ss << " [auto-configured]";
                }
                
                ss << std::endl;
            }
        }
    } else {
        ss << "No active profile" << std::endl;
    }
    
    return ss.str();
}

void ConfigurationManager::applyConfiguration() {
    if (!resource_manager_ || !current_profile_) {
        return;
    }
    
    // Apply allocation mode
    std::string allocation_mode = current_profile_->getParameter("allocation_mode");
    if (!allocation_mode.empty()) {
        if (allocation_mode == "auto") {
            resource_manager_->setAllocationMode(resource::AllocationMode::AUTO);
        } else if (allocation_mode == "cpu_only") {
            resource_manager_->setAllocationMode(resource::AllocationMode::CPU_ONLY);
        } else if (allocation_mode == "gpu_only") {
            resource_manager_->setAllocationMode(resource::AllocationMode::GPU_ONLY);
        } else if (allocation_mode == "cuda_only") {
            resource_manager_->setAllocationMode(resource::AllocationMode::CUDA_ONLY);
        } else if (allocation_mode == "metal_only") {
            resource_manager_->setAllocationMode(resource::AllocationMode::METAL_ONLY);
        } else if (allocation_mode == "hybrid") {
            resource_manager_->setAllocationMode(resource::AllocationMode::HYBRID);
        }
    }
    
    // Apply MFP method
    std::string mfp_method = current_profile_->getParameter("mfp_method");
    if (!mfp_method.empty()) {
        if (mfp_method == "auto") {
            resource_manager_->setMFPMethod(resource::MFPMethod::AUTO);
        } else if (mfp_method == "method1") {
            resource_manager_->setMFPMethod(resource::MFPMethod::METHOD_1);
        } else if (mfp_method == "method2") {
            resource_manager_->setMFPMethod(resource::MFPMethod::METHOD_2);
        } else if (mfp_method == "method3") {
            resource_manager_->setMFPMethod(resource::MFPMethod::METHOD_3);
        }
    }
    
    // Apply performance logging
    std::string performance_logging = current_profile_->getParameter("performance_logging");
    if (!performance_logging.empty()) {
        bool enable = (performance_logging == "true" || performance_logging == "yes" || 
                      performance_logging == "1" || performance_logging == "on");
        resource_manager_->setPerformanceLogging(enable);
    }
    
    // TODO: Apply other configuration parameters as needed
}

HardwareClass ConfigurationManager::classifyHardware() {
    if (!resource_manager_) {
        return HardwareClass::CUSTOM;
    }
    
    // Run benchmark to evaluate hardware performance
    resource::BenchmarkResult benchmark = resource_manager_->runBenchmark();
    
    // Get system information
    std::string system_info = resource_manager_->getSystemInfo();
    
    // Parse CPU cores from system info
    int cpu_cores = 0;
    size_t pos = system_info.find("Physical Cores:");
    if (pos != std::string::npos) {
        std::string cores_str = system_info.substr(pos + 15);
        cores_str = cores_str.substr(0, cores_str.find('\n'));
        try {
            cpu_cores = std::stoi(cores_str);
        } catch (...) {
            // Ignore conversion errors
        }
    }
    
    // Parse total memory from system info
    double total_memory_gb = 0.0;
    pos = system_info.find("Total Memory:");
    if (pos != std::string::npos) {
        std::string memory_str = system_info.substr(pos + 13);
        memory_str = memory_str.substr(0, memory_str.find("GB"));
        try {
            total_memory_gb = std::stod(memory_str);
        } catch (...) {
            // Ignore conversion errors
        }
    }
    
    // Check if GPU is available
    bool has_gpu = system_info.find("No GPUs detected") == std::string::npos;
    
    // Classify hardware based on CPU cores, memory, and GPU availability
    if (cpu_cores >= 16 && total_memory_gb >= 64 && has_gpu) {
        // High-end server or workstation
        if (system_info.find("Xeon") != std::string::npos || 
            system_info.find("EPYC") != std::string::npos) {
            return HardwareClass::SERVER;
        } else {
            return HardwareClass::WORKSTATION;
        }
    } else if (cpu_cores >= 8 && total_memory_gb >= 16 && has_gpu) {
        return HardwareClass::HIGH_END;
    } else if (cpu_cores >= 4 && total_memory_gb >= 8) {
        return HardwareClass::MID_RANGE;
    } else {
        return HardwareClass::LOW_END;
    }
}

void ConfigurationManager::createDefaultProfiles() {
    // Create default profile
    createDefaultProfile("default", HardwareClass::CUSTOM);
    
    // Create hardware-specific profiles
    createDefaultProfile("low_end", HardwareClass::LOW_END);
    createDefaultProfile("mid_range", HardwareClass::MID_RANGE);
    createDefaultProfile("high_end", HardwareClass::HIGH_END);
    createDefaultProfile("server", HardwareClass::SERVER);
    createDefaultProfile("workstation", HardwareClass::WORKSTATION);
    
    // Configure hardware-specific profiles
    configureForLowEndHardware(profiles_["low_end"].get());
    configureForMidRangeHardware(profiles_["mid_range"].get());
    configureForHighEndHardware(profiles_["high_end"].get());
    configureForServerHardware(profiles_["server"].get());
    configureForWorkstationHardware(profiles_["workstation"].get());
}

void ConfigurationManager::registerDefaultParameters() {
    // Register allocation mode parameter
    registerParameter(
        "allocation_mode",
        ParamType::ENUM,
        "Resource allocation mode",
        "auto",
        {"auto", "cpu_only", "gpu_only", "cuda_only", "metal_only", "hybrid"}
    );
    
    // Register MFP method parameter
    registerParameter(
        "mfp_method",
        ParamType::ENUM,
        "MFP method to use",
        "auto",
        {"auto", "method1", "method2", "method3"}
    );
    
    // Register performance logging parameter
    registerParameter(
        "performance_logging",
        ParamType::BOOLEAN,
        "Enable performance logging",
        "true"
    );
    
    // Register thread count parameter
    registerParameter(
        "thread_count",
        ParamType::INTEGER,
        "Number of threads to use (0 = auto)",
        "0"
    );
    
    // Register memory limit parameter
    registerParameter(
        "memory_limit_mb",
        ParamType::INTEGER,
        "Memory limit in MB (0 = no limit)",
        "0"
    );
    
    // Register block size parameter
    registerParameter(
        "block_size",
        ParamType::INTEGER,
        "Block size for MFP method 3",
        "1024"
    );
    
    // Register verification level parameter
    registerParameter(
        "verification_level",
        ParamType::INTEGER,
        "Verification level (0-3)",
        "1"
    );
    
    // Register cache size parameter
    registerParameter(
        "cache_size_mb",
        ParamType::INTEGER,
        "Cache size in MB",
        "128"
    );
    
    // Register precision parameter
    registerParameter(
        "precision",
        ParamType::ENUM,
        "Numeric precision",
        "double",
        {"float", "double", "extended"}
    );
    
    // Register optimization level parameter
    registerParameter(
        "optimization_level",
        ParamType::INTEGER,
        "Optimization level (0-3)",
        "2"
    );
}

ConfigProfile* ConfigurationManager::createDefaultProfile(const std::string& name, HardwareClass hardware_class) {
    // Create profile
    auto profile = std::make_unique<ConfigProfile>(name, hardware_class);
    
    // Set default parameters
    for (const auto& param : parameter_definitions_) {
        profile->setParameter(param.first, param.second.default_value, false);
    }
    
    // Add profile to map
    auto result = profile.get();
    profiles_[name] = std::move(profile);
    
    return result;
}

void ConfigurationManager::configureForLowEndHardware(ConfigProfile* profile) {
    if (!profile) {
        return;
    }
    
    // Configure for low-end hardware
    profile->setParameter("allocation_mode", "cpu_only", true);
    profile->setParameter("mfp_method", "method1", true);
    profile->setParameter("thread_count", "2", true);
    profile->setParameter("memory_limit_mb", "1024", true);
    profile->setParameter("block_size", "512", true);
    profile->setParameter("verification_level", "1", true);
    profile->setParameter("cache_size_mb", "64", true);
    profile->setParameter("precision", "double", true);
    profile->setParameter("optimization_level", "1", true);
}

void ConfigurationManager::configureForMidRangeHardware(ConfigProfile* profile) {
    if (!profile) {
        return;
    }
    
    // Configure for mid-range hardware
    profile->setParameter("allocation_mode", "auto", true);
    profile->setParameter("mfp_method", "method2", true);
    profile->setParameter("thread_count", "4", true);
    profile->setParameter("memory_limit_mb", "4096", true);
    profile->setParameter("block_size", "1024", true);
    profile->setParameter("verification_level", "1", true);
    profile->setParameter("cache_size_mb", "128", true);
    profile->setParameter("precision", "double", true);
    profile->setParameter("optimization_level", "2", true);
}

void ConfigurationManager::configureForHighEndHardware(ConfigProfile* profile) {
    if (!profile) {
        return;
    }
    
    // Configure for high-end hardware
    profile->setParameter("allocation_mode", "hybrid", true);
    profile->setParameter("mfp_method", "method3", true);
    profile->setParameter("thread_count", "8", true);
    profile->setParameter("memory_limit_mb", "8192", true);
    profile->setParameter("block_size", "2048", true);
    profile->setParameter("verification_level", "2", true);
    profile->setParameter("cache_size_mb", "256", true);
    profile->setParameter("precision", "double", true);
    profile->setParameter("optimization_level", "3", true);
}

void ConfigurationManager::configureForServerHardware(ConfigProfile* profile) {
    if (!profile) {
        return;
    }
    
    // Configure for server hardware
    profile->setParameter("allocation_mode", "cpu_only", true);
    profile->setParameter("mfp_method", "method3", true);
    profile->setParameter("thread_count", "32", true);
    profile->setParameter("memory_limit_mb", "65536", true);
    profile->setParameter("block_size", "4096", true);
    profile->setParameter("verification_level", "3", true);
    profile->setParameter("cache_size_mb", "1024", true);
    profile->setParameter("precision", "double", true);
    profile->setParameter("optimization_level", "3", true);
}

void ConfigurationManager::configureForWorkstationHardware(ConfigProfile* profile) {
    if (!profile) {
        return;
    }
    
    // Configure for workstation hardware
    profile->setParameter("allocation_mode", "hybrid", true);
    profile->setParameter("mfp_method", "method3", true);
    profile->setParameter("thread_count", "16", true);
    profile->setParameter("memory_limit_mb", "32768", true);
    profile->setParameter("block_size", "4096", true);
    profile->setParameter("verification_level", "2", true);
    profile->setParameter("cache_size_mb", "512", true);
    profile->setParameter("precision", "double", true);
    profile->setParameter("optimization_level", "3", true);
}

// Helper function to convert ParamType to string
std::string paramTypeToString(ParamType type) {
    switch (type) {
        case ParamType::INTEGER:
            return "Integer";
        case ParamType::FLOAT:
            return "Float";
        case ParamType::BOOLEAN:
            return "Boolean";
        case ParamType::STRING:
            return "String";
        case ParamType::ENUM:
            return "Enum";
        default:
            return "Unknown";
    }
}

} // namespace config
} // namespace mfp
