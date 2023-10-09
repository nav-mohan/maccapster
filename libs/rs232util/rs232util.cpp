#include "rs232util.hpp"
#include <iostream>

void RS232Util::open_pin()
{
    // std::cout << "OPENING PIN!\n";
    fd_ = open(PORT_CU, O_RDWR | O_NOCTTY | O_NDELAY); // Open Serial Port
    lock_ = flock(fd_, LOCK_EX | LOCK_NB);
    if(lock_ != 0)
    {
        printf("Port is busy. You could try disconnecting and reconnecting the USB-RS232 connector\n");
        printf("You might also have to kill any processes that are currently using that port\n");
    }
}

void RS232Util::set_pin()
{
    // std::cout << "SETTING PIN! " << lock_ << std::endl;
    if(lock_!=0)
        open_pin();

    ioctl(fd_, TIOCSDTR, &flagDTR_); // Set DTR pin. We dont actually need to do this the first time we open the port.
}

void RS232Util::clear_pin()
{
    // std::cout << "CLEARING PIN!\n";
    ioctl(fd_, TIOCCDTR, &flagRTS_); // Clear DTR pin
}

void RS232Util::close_pin()
{
    // std::cout << "CLOSING PIN!\n";
    close(fd_); // this sets the DTR pin back to zero as well.
    lock_ = flock(fd_, LOCK_UN);
}