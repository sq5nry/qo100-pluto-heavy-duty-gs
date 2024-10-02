#!/bin/bash

DATA_FILE=~/humidity.csv
echo $(date),$(humidity/myenv/bin/python3 humidity/reporter.py) >> ${DATA_FILE}
