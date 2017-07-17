#!/bin/bash
echo "Checking for conf:"
while [ ! -f conf ]; do
	sleep 1s
done
sleep 5s
echo "Conf exists."
