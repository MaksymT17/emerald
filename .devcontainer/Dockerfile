# Get the base Ubuntu image
FROM ubuntu:23.04

ENV TZ=Europe/Kiev
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Update apps on the base image
RUN apt-get -y update && apt-get install -y

# Install am related dependencies & gtk3
RUN apt -y install git g++ cmake python3 valgrind gcovr libsqlite3-dev libjpeg-dev alsa-utils libgtk-3-0 libgtk-3-dev