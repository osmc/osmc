user=$(whoami)
if [ "$user" != "osmc" ]; then return 0; fi
shadow_entry=$(sudo getent shadow osmc)
pwd_hash_method=$(echo $shadow_entry|cut -d '$' -f 2)
pwd_hash_salt=$(echo $shadow_entry|cut -d '$' -f 3)
pwd_hash_rest=$(echo $shadow_entry|cut -d '$' -f 4)
pwd_hash=$(echo $pwd_hash_rest|cut -d ':' -f 1)
openssl_hash=$(echo $(openssl passwd -$pwd_hash_method -salt $pwd_hash_salt $user)|cut -d '$' -f 4)
if systemctl is-active ssh -q && [ "$pwd_hash" = "$openssl_hash" ] && [ ! -f /home/osmc/.nosshwarn ]
then
    echo "Warning: You are using SSH with the default OSMC credentials."
    echo "This is a security risk!"
    echo "To change your password, type passwd"
    echo "To disable this warning, type touch /home/osmc/.nosshwarn"
fi
