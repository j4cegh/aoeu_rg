echo "un4seen bass library installer by aoeu Mar. 16 2021"
echo "Downloading bass and bass_fx from un4seen servers"
mkdir -p /tmp/bass_install_sh/bass /tmp/bass_install_sh/bass_fx
curl http://us.un4seen.com/files/bass24-osx.zip --output /tmp/bass_install_sh/bass.zip
curl http://us.un4seen.com/files/z/0/bass_fx24-osx.zip --output /tmp/bass_install_sh/bass_fx.zip
unzip /tmp/bass_install_sh/bass.zip -d /tmp/bass_install_sh/bass/.
unzip /tmp/bass_install_sh/bass_fx.zip -d /tmp/bass_install_sh/bass_fx/.
sudo cp /tmp/bass_install_sh/bass/bass.h /usr/local/include/
sudo cp /tmp/bass_install_sh/bass/libbass.dylib /usr/local/lib/
sudo cp /tmp/bass_install_sh/bass_fx/bass_fx.h /usr/local/include/
sudo cp /tmp/bass_install_sh/bass_fx/libbass_fx.dylib /usr/local/lib/
echo "Installed."
echo "Deleting temp files at /tmp/bass_install_sh/..."
rm -rf /tmp/bass_install_sh/