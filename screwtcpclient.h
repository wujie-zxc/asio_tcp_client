#ifndef SCREWTCPCLIENT_H
#define SCREWTCPCLIENT_H

#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <asio.hpp>
#include <QDateTime>
#include <QtDebug>

using asio::io_context;
using asio::ip::tcp;

enum class eStatus : uint16_t
{
    STATE_IDLE           = 0x00,   // 初始化状态
    STATE_START_SCREW = 0x01,   // 启动锁紧
    STATE_FIT             = 0x02,   // 咬合
    STATE_ROTATE        = 0x03,   // 旋入
    STATE_SCREW         = 0x04,   // 锁紧
    STATE_HOLD           = 0x05,   // 保持
    STATE_FINISH          = 0x06,   // 完成
    STATE_ERROR              = 0x07,   // 故障
    STATE_START_UNSCREW   =0x08,   // 启动松螺钉
    STATE_UNSCREWING       =0x09,   // 正在松螺钉
};

enum class eError : uint16_t
{
    NONE = 0x00,
    FIT_TIMEOUT                 = 0x01,    // 咬合超时
    ROTATE_UNDER_LOWER_LIMIT   = 0x02,    // 未旋入到位（下限）
    ROTATE_TIMEOUT              = 0x04,    // 旋入超时
    SCREW_TIMEOUT               = 0x08,    // 锁紧超时
    HOLD_TIMEOUT                = 0x10,    // 保持超时
    UNSCREW_TIMEOUT            = 0x20,    // 旋出超时
    INVALID_PARAMETER          = 0x40,    // 参数配置非法
};

struct Status
{
    QDateTime datetime;
    double vol{0.f};
    double temp{0.f};
    int32_t sys_cur{0};
    int32_t sys_speed{0};
    int32_t sys_pos{0};
    int16_t screwlock_ctl_word{0};
    double actual_torque{0.f};
    eStatus screw_status{eStatus::STATE_IDLE};
    eError err{eError::NONE};

    Status() = default;
    Status(Status&& other) noexcept
        : datetime(std::move(other.datetime)),
          vol(other.vol),
          temp(other.temp),
          sys_cur(other.sys_cur),
          sys_speed(other.sys_speed),
          sys_pos(other.sys_pos),
          screwlock_ctl_word(other.screwlock_ctl_word),
          actual_torque(other.actual_torque),
          screw_status(other.screw_status),
          err(other.err)
    {
    }

    Status& operator=(Status&& other) noexcept
    {
        if (this != &other)
        {
            datetime = std::move(other.datetime);
            vol = other.vol;
            temp = other.temp;
            sys_cur = other.sys_cur;
            sys_speed = other.sys_speed;
            sys_pos = other.sys_pos;
            screwlock_ctl_word = other.screwlock_ctl_word;
            actual_torque = other.actual_torque;
            screw_status = other.screw_status;
            err = other.err;
        }
        return *this;
    }
};

class ScrewTcpClient : public std::enable_shared_from_this<ScrewTcpClient>
{
    using StateCallback = std::function<void(const  Status&)>;

public:
    ScrewTcpClient() noexcept;
    ~ScrewTcpClient();
    bool connect(const std::string& ip, const std::string &port);
    void disconnect();
    void setRealtimeCallBack(StateCallback callback) { call_back_ = callback; }

    void p_test(Status&status){
        QStringList statusStrings;
        statusStrings << "Datetime: " + status.datetime.toString("yyyy-MM-dd hh:mm:ss");
        statusStrings << "Vol: " + QString::number(status.vol);
        statusStrings << "Temp: " + QString::number(status.temp);
        statusStrings << "Sys Cur: " + QString::number(status.sys_cur);
        statusStrings << "Sys Speed: " + QString::number(status.sys_speed);
        statusStrings << "Sys Pos: " + QString::number(status.sys_pos);
        statusStrings << "Screwlock Ctl Word: " + QString::number(status.screwlock_ctl_word);
        statusStrings << "Actual Torque: " + QString::number(status.actual_torque);
        statusStrings << "Screw Status: " + QString::number(static_cast<std::underlying_type<eStatus>::type>(status.screw_status));
        statusStrings << "Error: " + QString::number(static_cast<std::underlying_type<eError>::type>(status.err));
        qInfo() << statusStrings;
    }


private:
    void handle_connect(const asio::error_code& error);
    void do_read();
    void handle_read(const asio::error_code& error, std::size_t bytes_transferred);


private:
    asio::streambuf buffer_;
    std::string read_data_;
    std::thread io_thread_;
    std::shared_ptr<io_context> io_{nullptr};
    std::shared_ptr<tcp::socket> socket_{nullptr};
    StateCallback call_back_{nullptr};
};

#endif // SCREWTCPCLIENT_H
