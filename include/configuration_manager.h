#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "resource_manager.h"

namespace mfp {
namespace config {

// Forward declarations
class ConfigurationManager;
class ConfigProfile;

// Configuration parameter types
enum class ParamType {
    INTEGER,
    FLOAT,
    BOOLEAN,
    STRING,
    ENUM
};

// Configuration parameter
struct ConfigParameter {
    std::string name;
    ParamType type;
    std::string description;
    std::string default_value;
    std::vector<std::string> allowed_values; // For ENUM type
    std::string current_value;
    bool auto_configured;
};

// Hardware class (for configuration rules)
enum class HardwareClass {
    LOW_END,       // Basic hardware (e.g., dual-core CPU, no GPU)
    MID_RANGE,     // Moderate hardware (e.g., quad-core CPU, basic GPU)
    HIGH_END,      // High-performance hardware (e.g., 8+ core CPU, gaming GPU)
    SERVER,        // Server-grade hardware (e.g., 16+ core CPU, lots of RAM)
    WORKSTATION,   // Workstation hardware (e.g., high-end CPU, professional GPU)
    CUSTOM         // Custom hardware configuration
};

// Configuration profile
class ConfigProfile {
public:
    // Constructor and destructor
    ConfigProfile(const std::string& name, HardwareClass hardware_class);
    ~ConfigProfile() = default;
    
    // Get profile name
    std::string getName() const;
    
    // Get hardware class
    HardwareClass getHardwareClass() const;
    
    // Set parameter
    void setParameter(const std::string& name, const std::string& value, bool auto_configured = false);
    
    // Get parameter
    std::string getParameter(const std::string& name) const;
    
    // Check if parameter exists
    bool hasParameter(const std::string& name) const;
    
    // Get all parameters
    const std::map<std::string, ConfigParameter>& getParameters() const;
    
    // Load profile from file
    bool loadFromFile(const std::string& filename);
    
    // Save profile to file
    bool saveToFile(const std::string& filename) const;
    
    // Create string representation
    std::string toString() const;
    
private:
    std::string name_;
    HardwareClass hardware_class_;
    std::map<std::string, ConfigParameter> parameters_;
};

// Configuration manager
class ConfigurationManager {
public:
    // Constructor and destructor
    ConfigurationManager();
    ~ConfigurationManager();
    
    // Initialize configuration manager
    bool initialize(resource::ResourceManager* resource_manager);
    
    // Auto-configure based on detected hardware
    bool autoConfigureForHardware();
    
    // Load configuration from file
    bool loadConfiguration(const std::string& filename);
    
    // Save configuration to file
    bool saveConfiguration(const std::string& filename) const;
    
    // Get current profile
    ConfigProfile* getCurrentProfile();
    
    // Set current profile
    void setCurrentProfile(const std::string& profile_name);
    
    // Create new profile
    ConfigProfile* createProfile(const std::string& name, HardwareClass hardware_class);
    
    // Delete profile
    bool deleteProfile(const std::string& name);
    
    // Get profile by name
    ConfigProfile* getProfile(const std::string& name);
    
    // Get all profiles
    const std::map<std::string, std::unique_ptr<ConfigProfile>>& getProfiles() const;
    
    // Register parameter
    void registerParameter(const std::string& name, ParamType type, const std::string& description, 
                          const std::string& default_value, 
                          const std::vector<std::string>& allowed_values = {});
    
    // Get parameter
    std::string getParameter(const std::string& name) const;
    
    // Set parameter
    void setParameter(const std::string& name, const std::string& value, bool auto_configured = false);
    
    // Get parameter as integer
    int getIntParameter(const std::string& name) const;
    
    // Get parameter as float
    float getFloatParameter(const std::string& name) const;
    
    // Get parameter as boolean
    bool getBoolParameter(const std::string& name) const;
    
    // Get parameter description
    std::string getParameterDescription(const std::string& name) const;
    
    // Get parameter type
    ParamType getParameterType(const std::string& name) const;
    
    // Get allowed values for parameter
    std::vector<std::string> getAllowedValues(const std::string& name) const;
    
    // Check if parameter is auto-configured
    bool isAutoConfigured(const std::string& name) const;
    
    // Reset parameter to default value
    void resetParameter(const std::string& name);
    
    // Reset all parameters to default values
    void resetAllParameters();
    
    // Get configuration summary
    std::string getConfigurationSummary() const;
    
    // Apply configuration to resource manager
    void applyConfiguration();
    
private:
    // Resource manager reference
    resource::ResourceManager* resource_manager_;
    
    // Configuration profiles
    std::map<std::string, std::unique_ptr<ConfigProfile>> profiles_;
    
    // Current profile
    ConfigProfile* current_profile_;
    
    // Parameter definitions
    std::map<std::string, ConfigParameter> parameter_definitions_;
    
    // Helper methods
    HardwareClass classifyHardware();
    void createDefaultProfiles();
    void registerDefaultParameters();
    ConfigProfile* createDefaultProfile(const std::string& name, HardwareClass hardware_class);
    void configureForLowEndHardware(ConfigProfile* profile);
    void configureForMidRangeHardware(ConfigProfile* profile);
    void configureForHighEndHardware(ConfigProfile* profile);
    void configureForServerHardware(ConfigProfile* profile);
    void configureForWorkstationHardware(ConfigProfile* profile);
};

} // namespace config
} // namespace mfp
