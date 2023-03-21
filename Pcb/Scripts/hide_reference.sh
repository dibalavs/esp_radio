file=$1
echo 1
#file=$(realpath $file)
echo 2
echo $file
#sed -i |"^(fp_text user \"\${REFERENCE}\" (at \.*) (layer \.*)$|\1 hide|g" $file
sed -r 's|(\(fp_text user \"\$\{REFERENCE\}\" \(at .*\) \(layer .*\)).*$|\1 hide|' < $file > $file.1

