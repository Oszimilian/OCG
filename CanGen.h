#ifndef _CAN_GEN_H_
#define _CAN_GEN_H_

#include <string>
#include <yaml-cpp/yaml.h>
#include <map>

struct BaseConfig {
    std::string interfaceName;
    std::string dbcFilePath;
    std::string customWaveFilePath;
};

struct Wave {
    std::map<int, int> multiWaveSteps;
    float value;
    float m;
};

struct WaveSignalConfig {
    int noise;
    Wave wave;
};

struct WaveMessageConfig {
    //Signalname - WaveSignalConfig
    std::map<std::string, WaveSignalConfig> signalConfig;
};

struct WaveConfig {
    //Messagename - WaveMessageConfig
    std::map<std::string, WaveMessageConfig> messages;
};





class CanGen {
    public:
        CanGen() = default;

        bool importBaseConfig(const std::string& baseConfigFilePath) {
            try {
                YAML::Node config = YAML::LoadFile(baseConfigFilePath);

                auto root = config["BaseConfig"];

                for (const auto& subNode : root) {
                    BaseConfig baseConfig;

                    baseConfig.interfaceName = subNode["interfaceName"].as<std::string>();
                    baseConfig.dbcFilePath = subNode["dbcFileName"].as<std::string>();
                    baseConfig.customWaveFilePath = subNode["customWaveFile"].as<std::string>();
                    
                    m_interfaceConfigMap.emplace(baseConfig.interfaceName, std::move(baseConfig));
                }

            }catch(const YAML::ParserException& ex){
                std::cerr << "Yaml File could not be parsed" << std::endl;
                std::cerr << "Filename: " << baseConfigFilePath << std::endl;
                return false;
            }

            return true;
        }

        void init() {
            for (const auto&[interfaceName, baseConfig] : m_interfaceConfigMap) {
                importWaveConfig(baseConfig.customWaveFilePath, interfaceName);
            }

            print();
        }

        bool isNumber(const std::string& val) {
            
            for (const char c : val) {
                if(c < '0' || c > '9') return false;
            }
            
            return true;
        }

        bool importWaveConfig(const std::string& waveConfigFilePath, const std::string& interfaceName) {
            try 
            {
                YAML::Node config = YAML::LoadFile(waveConfigFilePath);
                
                auto root = config["waveConfig"];
                
                auto updateDuration = root["updateDuration"];
                m_gloablUpdateDurationUnit = updateDuration["unit"].as<std::string>();
                m_globalUpdateDuration = updateDuration["duration"].as<int>();

                WaveConfig waveSingleConfig;
                  
                auto single = root["single"];

                
                for (const auto& canMessage : single) {
                    WaveMessageConfig waveMessageConfig;

                    auto messageName = canMessage["messageName"].as<std::string>();

                    

                    for (const auto& canSignals : canMessage["messageSignals"]) {
                        WaveSignalConfig waveSignalConfig;
                        
                        auto signalName = canSignals["signalName"].as<std::string>();
                        
                        waveSignalConfig.noise = canSignals["signalNoise"].as<int>();
                        
                        Wave wave;
                        for (const auto& steps : canSignals["wave"]) {
                            
                            std::string leftNumber = steps.first.as<std::string>();

                            if(isNumber(leftNumber)) {
                                int rawNumber = atoi(leftNumber.c_str());
                                int value = steps.second.as<int>();
                                wave.multiWaveSteps.emplace(rawNumber, value);
                            }
                        }
                        
                        waveSignalConfig.wave = wave;

                        waveMessageConfig.signalConfig.emplace(signalName, waveSignalConfig);
                    }
                    
                    waveSingleConfig.messages.emplace(messageName, waveMessageConfig);
                    
                }
            
                auto multi = root["multi"];

                WaveSignalConfig waveSignalConfig;
                waveSignalConfig.noise = multi["signalNoise"].as<int>();

                Wave wave;
                for (const auto& steps : multi["wave"]) {
                    std::string leftNumber = steps.first.as<std::string>();
                    if(isNumber(leftNumber)) {
                        int rawNumber = atoi(leftNumber.c_str());
                        int value = steps.second.as<int>();
                        wave.multiWaveSteps.emplace(rawNumber, value);
                    }
                }
                waveSignalConfig.wave = wave;

                for (const auto& parts : multi["parts"]) {
                    WaveMessageConfig waveMessageConfig;
                    std::string messageName = parts["messageName"].as<std::string>();
                    for (const auto& signal : parts["messageSignals"]) {
                        std::string signalsName = signal.as<std::string>();
                        waveMessageConfig.signalConfig.emplace(signalsName, waveSignalConfig);
                    }
                    waveSingleConfig.messages.emplace(messageName, waveMessageConfig);
                }

                m_waveSingleConfigMap.emplace(interfaceName, waveSingleConfig);

            }catch(const YAML::ParserException& ex){
                std::cerr << "Yaml File could not be parsed" << std::endl;
                std::cerr << "Filename: " << waveConfigFilePath << std::endl;
                return false;
            }

            return true;
        }

        void print() {
            for (const auto& [interfaceName, waveSingleConfig] : m_waveSingleConfigMap) {
                std::cout << "InterfaceName: " << interfaceName << std::endl;
                for (const auto& [messageName, waveMessageConfig] : waveSingleConfig.messages) {
                    std::cout << "\t" << "MessageName: " << messageName << std::endl;
                    for (const auto& [signalName, waveSignalConfig] : waveMessageConfig.signalConfig) {
                        std::cout << "\t\t" << "SignalName: " << signalName << std::endl;
                        std::cout << "\t\t\t" << "Noise: " << waveSignalConfig.noise << std::endl;
                        for (const auto& [count, value] : waveSignalConfig.wave.multiWaveSteps) {
                            std::cout << "\t\t\t\t" << "count(" << count << ") = " << value << std::endl; 
                        }
                    }
                }
            }
        }

    private:
        std::map<std::string, BaseConfig> m_interfaceConfigMap;
        std::map<std::string, WaveConfig> m_waveSingleConfigMap;

    private:
        int m_globalUpdateDuration;
        std::string m_gloablUpdateDurationUnit;
};

#endif