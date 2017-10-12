export ARCH=arm CROSS_COMPILE=~/arm-eabi-4.8/bin/arm-eabi-
#export KBUILD_BUILD_USER=assusdan
#export KBUILD_BUILD_HOST=SRT

    #For checking errors
echo 'Remove kernel...'
rm -rf arch/arm/boot/zImage


echo 'Configure CM Zera S '
make alps_defconfig >/dev/null

echo 'Building CM Zera S'
make -j4 zImage >/dev/null 2>buildlog.log

    #check errors
if [ ! -f arch/arm/boot/zImage ]
then
    echo "BUID ERRORS!"
else
 #if OK
echo 'Moving D3'
mv arch/arm/boot/zImage /var/www/html/d3-boot.img-kernel
fi


#write worktime
echo $[$SECONDS / 60]' minutes '$[$SECONDS % 60]' seconds' 
