#!/bin/bash

## Tests whether the USB-CEC adapter can be accessed correctly
## Copyright (C) 2011-2013 Pulse-Eight Ltd.

check_lsusb()
{
  echo -n "  * searching USB device:             "
  adapters_found=`lsusb | grep 2548:1001 | wc -l`
  if [ "x${adapters_found}" = "x0" ]; then
    echo "NOT FOUND"
    return 1
  else
    echo "ok"
    return 0
  fi
}

check_acm_module()
{
  echo -n "  * checking for CDC-ACM support:     "
  acm_found=`grep cdc_acm /proc/modules | wc -l`
  if [ "x${acm_found}" = "x0" ]; then
    echo "NOT LOADED"
    return 1
  else
    echo "ok"
    return 0
  fi
}

check_acm_file()
{
  echo -n "  * checking for CDC-ACM node:        "
  acm_found=`ls /dev/ttyACM* | wc -l`
  if [ "x${acm_found}" = "x0" ]; then
    echo "NOT FOUND"
    return 1
  else
    echo "ok"
    return 0
  fi
}

check_cec_client_dev()
{
  echo -n "  * checking cec-client:              "
  cec_client=`cec-client -l | grep 'Found devices' | awk '{print \$3}'`
  if [ -z "${cec_client}" ]; then
    echo "ERROR"
  elif [ "x${cec_client}" = "xNONE" ]; then
    echo "NO DEVICES FOUND"
  else
    echo "ok"
  fi
}

check_poll_tv()
{
  echo -n "  * trying to poll the TV:            "
  cec_client=`echo 'poll 0' | cec-client -t p -p 1 -d 1 -s | tail -n1 | grep 'POLL'`
  if [ -z "${cec_client}" ]; then
    echo "ERROR"
  elif [ "x${cec_client}" = "xPOLL message sent" ]; then
    echo "ok"
  else
    echo "COULD NOT POLL THE TV"
  fi
}

check_tv_vendor()
{
  echo -n "  * vendor id of the TV:              "
  cec_client=`echo 'ven 0' | cec-client -t p -p 1 -d 1 -s | tail -n1 | grep 'vendor' | awk '{print \$3}'`
  if [ -z "${cec_client}" ]; then
    echo "ERROR"
  else
    echo "${cec_client}"
  fi
}

check_tv_power()
{
  echo -n "  * power status of the TV:           "
  cec_client=`echo 'pow 0' | cec-client -t p -p 1 -d 1 -s | tail -n1 | grep 'power' | awk '{print \$3}'`
  if [ -z "${cec_client}" ]; then
    echo "ERROR"
  else
    echo "${cec_client}"
  fi
}

check_tv_lang()
{
  echo -n "  * language of the TV:               "
  cec_client=`echo 'lang 0' | cec-client -t p -p 1 -d 1 -s | tail -n1 | grep 'language' | awk '{print \$3}'`
  if [ -z "${cec_client}" ]; then
    echo "ERROR"
  else
    echo "${cec_client}"
  fi
}

send_power_off()
{
  echo -n "  * powering off the TV:              "
  cec_client=`echo 'standby 0' | cec-client -t p -p 1 -d 1 -s | tail -n1`
  echo "ok"
}

echo "Pulse-Eight USB-CEC Adapter tester v0.1"
echo ""

check_lsusb
check_acm_module
check_acm_file
check_cec_client_dev
check_poll_tv
check_tv_vendor
check_tv_power
check_tv_lang
send_power_off

