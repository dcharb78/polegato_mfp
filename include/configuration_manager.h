#pragma once

#include <string>
#include <map>
#include <memory>
#include "resource_manager.h"

namespace mfp {

// Configuration profile types
enum class ProfileType {
    LOW_END,     // Low-end hardware (dual-core CPUs, limited RAM)
    MID_RANGE,   // Mid-range hardware (quad-core CPUs, moderate RAM)
    HIGH_END,    // High-end hardware (8+ core CPUs, gaming GPUs)
    SERVER,      // Server hardware (16+ core CPUs, large RAM)
    WORKSTATION, // Workstation hardware (high-end CPUs, professional GPUs)
    CUSTOM       // Custom configuration
};

// Configuration parameter types
enum class ParamType {
    INT,         // Integer parameter
    DOUBLE,      // Double parameter
    BOOL,        // Boolean parameter
    STRING,      // String parameter
    ENUM         // Enumeration parameter
};

// Configuration parameter
struct ConfigParam {
    std::string name;
    ParamType type;
    
    // Value storage (only one is used based on type)
    int int_value;
    double double_value;
    bool bool_value;
    std::string string_value;
    
    // Constructor for int parameter
    ConfigParam(const std::string& n, int value) 
        : name(n), type(ParamType::INT), int_value(value) {}
    
    // Constructor for double parameter
    ConfigParam(const std::string& n, double value) 
        : name(n), type(ParamType::DOUBLE), double_value(value) {}
    
    // Constructor for bool parameter
    ConfigParam(const std::string& n, bool value) 
        : name(n), type(ParamType::BOOL), bool_value(value) {}
    
    // Constructor for string parameter
    ConfigParam(const std::string& n, const std::string& value) 
        : name(n), type(ParamType::STRING), string_value(value) {}
    
    // Constructor for enum parameter (stored as string)
    ConfigParam(const std::string& n, const std::string& value, ParamType t) 
        : name(n), type(t), string_value(value) {}
};

// Configuration profile
class ConfigProfile {
public:
    ConfigProfile(ProfileType type, const std::string& name);
    
    // Get profile type
    ProfileType getType() const;
    
    // Get profile name
    const std::string& getName() const;
    
    // Set parameter
    void setParam(const std::string& name, const ConfigParam& param);
    
    // Get parameter
    const ConfigParam* getParam(const std::string& name) const;
    
    // Apply profile to resource manager
    void apply(ResourceManager& resource_manager) const;
    
    // Create profile from resource manager
    static ConfigProfile createFromResourceManager(const ResourceManager& resource_manager, const std::string& name);
    
    // Create default profile for hardware
    static ConfigProfile createDefaultProfile(const ResourceManager& resource_manager);
    
private:
    ProfileType m_type;
    std::string m_name;
    std::map<std::string, ConfigParam> m_params;
};

// Configuration manager class
class ConfigurationManager {
public:
    ConfigurationManager();
    ~ConfigurationManager();
    
    // Initialize configuration manager
    bool initialize(ResourceManager& resource_manager);
    
    // Get current profile
    const ConfigProfile& getCurrentProfile() const;
    
    // Set current profile
    void setCurrentProfile(const ConfigProfile& profile);
    
    // Get available profiles
    const std::map<std::string, ConfigProfile>& getProfiles() const;
    
    // Add profile
    void addProfile(const ConfigProfile& profile);
    
    // Remove profile
    void removeProfile(const std::string& name);
    
    // Save configuration to file
    bool saveToFile(const std::string& filename) const;
    
    // Load configuration from file
    bool loadFromFile(const std::string& filename);
    
    // Get configuration summary
    std::string getConfigurationSummary() const;
    
private:
    ResourceManager* m_resource_manager;
    ConfigProfile m_current_profile;
    std::map<std::string, ConfigProfile> m_profiles;
    
    // Create default profiles
    void createDefaultProfiles();
};

// Global configuration manager instance
ConfigurationManager& getConfigurationManager();

} // namespace mfp
