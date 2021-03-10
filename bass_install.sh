#http://www.un4seen.com/download.php?bass24-linux
#http://www.un4seen.com/download.php?z/0/bass_fx24-linux
#I suck at bash...

echo "un4seen bass library installer by aoeu Mar. 10 2021"
echo "Make sure cURL and unzip are installed."
echo "I'm assuming you're on a x86_64 system, if not I'm sorry"
echo "Downloading bass and bass_fx from un4seen servers"
mkdir -p /tmp/bass_install_sh/bass /tmp/bass_install_sh/bass_fx
curl http://us.un4seen.com/files/bass24-linux.zip --output /tmp/bass_install_sh/bass.zip
curl http://us.un4seen.com/files/z/0/bass_fx24-linux.zip --output /tmp/bass_install_sh/bass_fx.zip
unzip /tmp/bass_install_sh/bass.zip -d /tmp/bass_install_sh/bass/.
unzip /tmp/bass_install_sh/bass_fx.zip -d /tmp/bass_install_sh/bass_fx/.
sudo cp /tmp/bass_install_sh/bass/bass.h /usr/include/
sudo cp /tmp/bass_install_sh/bass/x64/libbass.so /usr/lib/
sudo cp /tmp/bass_install_sh/bass_fx/C/bass_fx.h /usr/include/
sudo cp /tmp/bass_install_sh/bass_fx/x64/libbass_fx.so /usr/lib/
echo "Installed."
echo "Deleting temp files at /tmp/bass_install_sh/..."
rm -rf /tmp/bass_install_sh/