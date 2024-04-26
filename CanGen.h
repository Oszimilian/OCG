#ifndef _CAN_GEN_H_
#define _CAN_GEN_H_

#include <string>
#include <yaml-cpp/yaml.h>
#include <map>
#include <boost/asio.hpp>
#include <algorithm>

struct BaseConfig {
    std::string interfaceName;
    std::string dbcFilePath;
    std::string customWaveFilePath;
};

enum class TransistionTyp : uint32_t{
    staticTransistion,
    linearTransistion
};

struct Wave {
    std::map<int, float> multiWaveSteps;
    float value;
    float m;
    TransistionTyp transistionTyp;
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
        CanGen(){}



        static bool importBaseConfig(const std::string& baseConfigFilePath) {
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

        static void processMessages() {

            for (auto& [interfaceName, waveConfig] : m_waveSingleConfigMap) {
                
                for (auto& [messageName, messageConfig] : waveConfig.messages){
                    
                    for (auto& [signalName, waveSignalConfig] : messageConfig.signalConfig) {

                        bool foundNewGlobalStep = false;
                        auto itMwsBegin = waveSignalConfig.wave.multiWaveSteps.begin();
                        auto itMwsEnd = waveSignalConfig.wave.multiWaveSteps.end();
                        for (; itMwsBegin != itMwsEnd; itMwsBegin++) {
                            if(itMwsBegin->first == m_globalStep) {
                                foundNewGlobalStep = true;
                                break;
                            }
                        }
                        if (foundNewGlobalStep) {
                            if(waveSignalConfig.wave.transistionTyp == TransistionTyp::staticTransistion) {
                                waveSignalConfig.wave.value = itMwsBegin->second;
                            }
                            else if(waveSignalConfig.wave.transistionTyp ==  TransistionTyp::linearTransistion) {
                                float y = itMwsBegin->second;
                                int x = itMwsBegin->first;
                                
                                itMwsBegin++;
                                if (itMwsBegin != itMwsEnd) {
                                    float deltaX = (float)(itMwsBegin->first - x);
                                    float deltaY = (float)(itMwsBegin->second - y);
                                    waveSignalConfig.wave.m = deltaY / deltaX;
                                    std::cout << "m = " << waveSignalConfig.wave.m << std::endl;
                                } else {
                                    waveSignalConfig.wave.transistionTyp = TransistionTyp::staticTransistion;
                                }

                                waveSignalConfig.wave.value = y;
                            }
                        } else {
                            if(waveSignalConfig.wave.transistionTyp ==  TransistionTyp::linearTransistion) {
                               waveSignalConfig.wave.value += waveSignalConfig.wave.m;
                            }
                        }


                        std::cout << signalName << ": " << waveSignalConfig.wave.value << std::endl;
                        
                    }
                    
                }
                
            }
            std::cout << std::endl;
            m_globalStep++;
        }

        static void timerCallback(const boost::system::error_code&, boost::asio::steady_timer* timer) {
            processMessages();
            timer->expires_at(timer->expiry() + boost::asio::chrono::milliseconds(m_globalUpdateDuration));
            timer->async_wait(std::bind(timerCallback, std::placeholders::_1, timer));
        }

        static void init() {
            for (const auto&[interfaceName, baseConfig] : m_interfaceConfigMap) {
                importWaveConfig(baseConfig.customWaveFilePath, interfaceName);
            }

            boost::asio::steady_timer timer(m_io, boost::asio::chrono::milliseconds(m_globalUpdateDuration));
            timer.async_wait(std::bind(timerCallback, std::placeholders::_1, &timer));

            print();

            std::cout << "Started Timer" << std::endl;
            m_io.run();
        }


        static bool isNumber(const std::string& val) {
            
            for (const char c : val) {
                if(c < '0' || c > '9') return false;
            }
            
            return true;
        }


        static bool importWaveConfig(const std::string& waveConfigFilePath, const std::string& interfaceName) {
            try 
            {
                YAML::Node config = YAML::LoadFile(waveConfigFilePath);
                
                auto root = config["waveConfig"];
                
                auto updateDuration = root["updateDuration"];
                m_gloablUpdateDurationUnit = updateDuration["unit"].as<std::string>();
                m_globalUpdateDuration = updateDuration["duration"].as<int>();


                WaveConfig waveSingleConfig;
                  
                auto single = root["single"];

                if (single) {
                    for (const auto& canMessage : single) {
                        WaveMessageConfig waveMessageConfig;

                        auto messageName = canMessage["messageName"].as<std::string>();

                        

                        for (const auto& canSignals : canMessage["messageSignals"]) {
                            WaveSignalConfig waveSignalConfig;
                            
                            auto signalName = canSignals["signalName"].as<std::string>();
                            
                            waveSignalConfig.noise = canSignals["signalNoise"].as<int>();


                            Wave wave;

                            auto transformType = canSignals["transformType"].as<std::string>();
                            if(transformType == "linear") {
                                wave.transistionTyp = TransistionTyp::linearTransistion;
                            } else if(transformType == "static") {
                                wave.transistionTyp = TransistionTyp::staticTransistion;
                            } else {
                                std::cerr << "unknown transformTyp" << std::endl;
                                std::cerr << "Typ: " << transformType << std::endl;
                                return false;
                            }
                            
                            
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
                }

            
                auto multi = root["multi"];

                if(multi) {
                    WaveSignalConfig waveSignalConfig;
                    waveSignalConfig.noise = multi["signalNoise"].as<int>();

                    Wave wave;

                    auto transformType = multi["transformType"].as<std::string>();
                    if(transformType == "linear") {
                        wave.transistionTyp = TransistionTyp::linearTransistion;
                    } else if(transformType == "static") {
                        wave.transistionTyp = TransistionTyp::staticTransistion;
                    } else {
                        std::cerr << "unknown transformTyp" << std::endl;
                        std::cerr << "Typ: " << transformType << std::endl;
                        return false;
                    }

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
                }

                m_waveSingleConfigMap.emplace(interfaceName, waveSingleConfig);
                

            }catch(const YAML::ParserException& ex){
                std::cerr << "Yaml File could not be parsed" << std::endl;
                std::cerr << "Filename: " << waveConfigFilePath << std::endl;
                return false;
            }

            return true;
        }

        static void print() {
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
        static std::map<std::string, BaseConfig> m_interfaceConfigMap;
        static std::map<std::string, WaveConfig> m_waveSingleConfigMap;


    public:
        static int m_globalStep;
        static int m_globalUpdateDuration;
        static std::string m_gloablUpdateDurationUnit;

    private:
        static boost::asio::io_context m_io;
};

std::map<std::string, BaseConfig> CanGen::m_interfaceConfigMap{};
std::map<std::string, WaveConfig> CanGen::m_waveSingleConfigMap{};
int CanGen::m_globalUpdateDuration = 10;
int CanGen::m_globalStep = 0;
std::string CanGen::m_gloablUpdateDurationUnit{};
boost::asio::io_context CanGen::m_io{};

#endif