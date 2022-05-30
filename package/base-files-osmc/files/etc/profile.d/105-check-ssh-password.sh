user=$(whoami)
if [ "$user" != "osmc" ]; then return 0; fi

shadow_entry=$(sudo getent shadow osmc)
pwd_hash_method=$(echo $shadow_entry|cut -d '$' -f 2)
shadow_password_field=$(echo $shadow_entry|cut -d ':' -f 2)

# check for yescrypt or use other hash-function 
if [ "$pwd_hash_method" = "y" ]
   then pwd_hash=$(echo $shadow_password_field|cut -d '$' -f 5)
   else pwd_hash=$(echo $shadow_password_field|cut -d '$' -f 4)
fi

pwd_method_n_salt=$(echo ${shadow_password_field%"\$$pwd_hash"})
python_password_field=$(echo $(python -c 'import crypt,sys; print(crypt.crypt(sys.argv[1], sys.argv[2]))' $user $pwd_method_n_salt))

if systemctl is-active ssh -q && [ "$shadow_password_field" = "$python_password_field" ] && [ ! -f /home/osmc/.nosshwarn ]
then
    echo "Warning: You are using SSH with the default OSMC credentials."
    echo "This is a security risk!"
    echo "To change your password, type passwd"
    echo "To disable this warning, type touch /home/osmc/.nosshwarn"
fi
