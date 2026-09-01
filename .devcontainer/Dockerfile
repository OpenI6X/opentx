# Dockerfile
FROM archlinux:base
RUN pacman -Syu --noconfirm

# Install misc packages
RUN pacman -S --noconfirm \
    7zip \
	arm-none-eabi-gcc \
	arm-none-eabi-newlib \
	avr-gcc \
	avr-libc \
	base-devel \
	bc \
	clang \
	cpio \
	curl \
	cmake \
	dos2unix \
	erofs-utils \
	git \
	less \
	nano \
	platformio-core \
	python \
	python-pip \
	python-pillow \
	python-lz4 \
	python-jinja \
	qt6-base \
	qt5-base \
	qt6-tools \ 
	qt6-translations \
	qt6-multimedia \
	qt6-serialport \
	qt6-svg \
	qt6-tools \
	screen \
	tree \
	unzip \
	vim \
	wget \
	zip

# HACK: Allow base-devel to run with root user
RUN sed -i '/E_ROOT/d' /usr/bin/makepkg

# Install yay
#RUN git clone https://aur.archlinux.org/yay-bin.git /tmp/yay-bin && \
#    cd /tmp/yay-bin && \
#	makepkg -si --noconfirm && \
#	cd && rm -rf /tmp/yay-bin

#RUN git clone --recursive https://github.com/STMicroelectronics/STM32CubeG0.git /workspace/STM32CubeG0
#RUN git clone --recursive https://github.com/STMicroelectronics/STM32CubeG4.git /workspace/STM32CubeG4

# Personal dir dotfiles
#ADD home /home/

# Personal uploader scripts
#ADD root/bin /usr/local/bin/

ARG USERNAME=csp
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME 

USER $USERNAME

