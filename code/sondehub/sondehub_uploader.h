#pragma once

#include <string>
#include <vector>
#include <mutex>

namespace sondehub
{

// minimal telemetry
struct MinTelemetry
{
    std::string payload_callsign;
    std::string time_received;
    std::string datetime;
    int frame;
    float lat;
    float lon;
    float alt;
};

class SondeHubUploader
{
private:
    std::vector<MinTelemetry>   queue_;
	mutable std::mutex          queue_mtx_;

    std::vector<MinTelemetry>   processing_queue_;

    std::string api_endpoint_;
    std::string uploader_callsign_;

public:
    SondeHubUploader(const std::string& api_endpoint, const std::string& uploader_callsign) : api_endpoint_(api_endpoint), uploader_callsign_(uploader_callsign) {};
    void push(const MinTelemetry&);
    void upload();
    size_t size() const;

};

}