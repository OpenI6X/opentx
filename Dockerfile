# Dockerfile
FROM archlinux:base
RUN pacman -Syu --noconfirm

# Install misc packages
RUN pacman -S --noconfirm \
    7zip \
	arm-none-eabi-gcc \
	arm-none-eabi-newlib \
	base-devel \
	bc \
	cmake \
	cpio \
	curl \
	dos2unix \
	erofs-utils \
	git \
	less \
	nano \
	python \
	python-pip \
	python-pilkit \
	screen \
	tree \
	unzip \
	vim \
	wget \
	zip

# HACK: Allow base-devel to run with root user
RUN sed -i '/E_ROOT/d' /usr/bin/makepkg

# Install yay
RUN git clone https://aur.archlinux.org/yay-bin.git /tmp/yay-bin && \
    cd /tmp/yay-bin && \
	makepkg -si --noconfirm && \
	cd && rm -rf /tmp/yay-bin

#RUN git clone --recursive https://github.com/STMicroelectronics/STM32CubeG0.git /workspace/STM32CubeG0
#RUN git clone --recursive https://github.com/STMicroelectronics/STM32CubeG4.git /workspace/STM32CubeG4

# Personal dir dotfiles
#ADD home /home/

# Personal uploader scripts
#ADD root/bin /usr/local/bin/
