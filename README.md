```
像不远处，盛夏里篝火的微光似的。
总以为只要不停地往前走，就能够回到过去。
```

## Hardware Prepare

This is an external module deployed on boards such as ARDUINO UNO R4. The board requires the installation of a W5500 ETH Ethernet expansion shield.

## Purpose of the Program

This program starts an HTTP coprocessor service, which can access sensor modules based on received requests and return responses.

## Usage
```bash
curl -X POST http://192.168.254.101:8001/api -d '{"cmd": "MPU6050"}'
```
