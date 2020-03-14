function [ret] = connect_smu()
%opens the serial port to the MCU on the main PCD and returns its serial
%device
 
delete(instrfindall) % closes all serial ports
% t = tcpip('192.168.178.59', 1000, 'NetworkRole', 'client')
% t.Timeout=1;
stm32=serial('COM3','BaudRate',1000000); %Konfiguriert Serial Port
stm32.Timeout = 10;
stm32.InputBufferSize = 3000;
fopen(stm32);

smu_ping(stm32);

ret = stm32;
end

