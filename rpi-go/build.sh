export GOPATH=/media/bags/gopath

rm -rf rpi-go

echo "build templates"
cd templates
$GOPATH/bin/qtc build
cd ..

echo "build rpi-go"
go build -o rpi-go

echo "running rpi-go"
./rpi-go
