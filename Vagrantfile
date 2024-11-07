# -*- mode: ruby -*-
# vi: set ft=ruby :

### Inspired by https://github.com/OSGeo/gdal/blob/master/Vagrantfile

require 'socket'

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  vm_ram = ENV['VAGRANT_VM_RAM'] || 1024
  vm_cpu = ENV['VAGRANT_VM_CPU'] || 1

  config.vm.box = "noble64"

  config.vm.hostname = "grass-gis-vagrant"
  config.vm.box = "alvistack/ubuntu-24.04"
  config.vm.define "grass-gis-vagrant" do |host|

    config.vm.network :forwarded_port, guest: 80, host: 8080

    host.vm.provider :virtualbox do |vb|
      vb.customize ["modifyvm", :id, "--memory", vm_ram]
      vb.customize ["modifyvm", :id, "--cpus", vm_cpu]
      vb.customize ["modifyvm", :id, "--ioapic", "on"]
      vb.name = "grass-gis-vagrant"
    end

    config.ssh.forward_agent = true
    config.ssh.forward_x11 = true

    ### Define your GRASS data directory to be synce'd on virtual machine
    # grassdata_dir = "/opt/grassdata"
    # host.vm.synced_folder grassdata_dir, "/home/vagrant/grassdata"

    ppaRepos = [
      # TODO: enable PPA & PDAL support when available
      # "ppa:ubuntugis/ubuntugis-unstable"
    ]

    packageList = [
      "autoconf2.69",
      "autotools-dev",
      "bison",
      "flex",
      "g++",
      "gettext",
      "libblas-dev",
      "libcairo2-dev",
      "libfftw3-dev",
      "libfreetype6-dev",
      "libgdal-dev",
      "libgeos-dev",
      "libglu1-mesa-dev",
      "libjpeg-dev",
      "liblapack-dev",
      "libmysqlclient-dev",
      "libncurses5-dev",
      "libnetcdf-dev",
      "libpng-dev",
      "libpq-dev",
      "libproj-dev",
      "libreadline-dev",
      "libsqlite3-dev",
      "libtiff-dev",
      "libxmu-dev",
      "make",
      "netcdf-bin",
      "proj-bin",
      "python3",
      "python3-dateutil",
      "python3-dev",
      "python3-numpy",
      "python3-pil",
      "python3-ply",
      "python3-wxgtk4.0",
      "unixodbc-dev",
      "zlib1g-dev",
      # "libpdal-dev"
    ]

    unless File.exists?(".no_apt_cache")
      cache_dir = "apt-cache/#{config.vm.box}"
      FileUtils.mkdir_p(cache_dir) unless Dir.exists?(cache_dir)
      host.vm.synced_folder cache_dir, "/var/cache/apt/archives"
    end

    if Dir.glob("#{File.dirname(__FILE__)}/.vagrant/machines/grass-gis-vagrant/*/id").empty?
      pkg_cmd = "sed -i 's#deb http://archive.ubuntu.com/ubuntu#deb mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list; "
      pkg_cmd << "apt-get update -qq; apt-get install -q -y python3-software-properties; "

      if ppaRepos.length > 0
	ppaRepos.each { |repo| pkg_cmd << "add-apt-repository -y " << repo << " ; " }
	pkg_cmd << "apt-get update -qq; "
      end

      # install packages we need we need
      pkg_cmd << "apt-get install -q -y " + packageList.join(" ") << " ; "
      host.vm.provision :shell, :inline => pkg_cmd

      scripts = [
      "clean.sh",
      ];
      scripts.each { |script| host.vm.provision :shell, :privileged => false, :path => "utils/vagrant/" << script }
    end
    scripts = [
      "compile.sh",
    ];
    scripts.each { |script| host.vm.provision :shell, :privileged => false, :path => "utils/vagrant/" << script }
  end
end
