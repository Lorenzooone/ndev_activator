cd build
unzip ${1}.zip 
codesign --deep -v -f -s "-" ${1}/ndev_activator.app
rm -f ${1}.zip
zip --symlinks -r ${1}.zip ${1}
