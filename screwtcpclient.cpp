#include "screwtcpclient.h"
#include "json.hpp"


const std::string PACK_BEGIN = "<PACK_BEGIN";
const std::string PACK_END = "PACK_END>";
const size_t PACK_BEGIN_LEN = PACK_BEGIN.length();
const size_t PACK_END_LEN = PACK_END.length();

static Status from_json(const nlohmann::json& js)
{
    Status obj;
    obj.screwlock_ctl_word = js.value("controlCode", obj.screwlock_ctl_word);
    obj.sys_cur = js.value("current", obj.sys_cur);
    obj.sys_pos = js.value("position", obj.sys_pos);
    obj.sys_speed = js.value("speed", obj.sys_speed);
    uint16_t status = js.value("statusCode", 0);
    obj.screw_status = static_cast<eStatus>(status & 0x1F);
    if (obj.screw_status == eStatus::STATE_ERROR) {
        obj.err = static_cast<eError>((status >> 5) & 0x7FF);
    }
    obj.temp = js.value("temperature", 0.0);
    obj.actual_torque = js.value("torque", 0.0);
    obj.vol = js.value("voltage", 0.0);
    obj.datetime = QDateTime::currentDateTime();
    return obj;
}


ScrewTcpClient::ScrewTcpClient() noexcept:
    io_(new asio::io_context),
    socket_(new asio::ip::tcp::socket(*io_))
{
}

ScrewTcpClient::~ScrewTcpClient()
{
    disconnect();
}

bool ScrewTcpClient::connect(const std::string &ip, const std::string &port)
{
    tcp::resolver resolver(*io_);
    tcp::resolver::results_type endpoints = resolver.resolve(ip, port);
    asio::async_connect(*socket_, endpoints,
                        std::bind(&ScrewTcpClient::handle_connect, shared_from_this(), std::placeholders::_1));


    io_thread_ = std::thread([this]() { io_->run(); });
    return true;
}

void ScrewTcpClient::disconnect()
{
    asio::post(*io_, [this]() {
        socket_->close();
    });

    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void ScrewTcpClient::handle_connect(const asio::error_code &error) {
    if (!error) {
        std::cout << "Connected to host." << std::endl;
        do_read();
    } else {
        std::cerr << "Connect to host failed: " << error.message() << std::endl;
    }
}

void ScrewTcpClient::do_read()
{
    asio::async_read_until(*socket_, buffer_, PACK_END,
                           std::bind(&ScrewTcpClient::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void ScrewTcpClient::handle_read(const asio::error_code& error, size_t bytes_transferred) {
    if (!error) {
        std::istream is(&buffer_);
        std::string line((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        buffer_.consume(bytes_transferred);
        read_data_ += line;

        size_t begin = 0, end = 0;
        while ((begin = read_data_.find(PACK_BEGIN)) != std::string::npos &&
               (end = read_data_.find(PACK_END, begin)) != std::string::npos) {
            if (end - begin >= (PACK_BEGIN_LEN + 8)) {
                std::string lenstr = read_data_.substr(begin + PACK_BEGIN_LEN, 8);
                int len = std::stoi(lenstr);
                size_t json_end = begin + PACK_BEGIN_LEN + 8 + len;
                if (json_end <= end) {
                    std::string jsonstr = read_data_.substr(begin + PACK_BEGIN_LEN + 8, len);
                    read_data_.erase(begin, end - begin + PACK_END_LEN);

                    try {
                        auto json = nlohmann::json::parse(jsonstr);
                        //qInfo() << QString::fromStdString(read_data_);
                       /* qInfo() << QDateTime::currentDateTime() << QString::fromStdString(json.dump());
                        qInfo() << "\n";
                        qInfo() << QString::fromStdString(read_data_).count();
                        qInfo() << "\n";*/
                        if (!json.is_object()) {
                            continue;
                        }
                        if (json.find("ScrewerStatus") == json.end()) {
                            std::cerr << "ScrewerStatus key not found in JSON" << std::endl;
                            continue;
                        }
                        auto s_js = json["ScrewerStatus"];
                        if (s_js.is_object()) {
                            auto objs = from_json(s_js);
                            // p_test(objs);
                            if (call_back_){
                                call_back_(objs);
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Parse realtime data error: " << e.what() << std::endl;
                        break;
                    }
                    catch(...){
                        break;
                    }
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
        do_read();
    }
    else {
        std::cerr << "Read error: " << error.message() << std::endl;
    }
}

