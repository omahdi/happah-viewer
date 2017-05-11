## Happah Viewer

### Contributing

New developers who wish to contribute to the project can get started by executing the following commands on an Ubuntu 16.04.2 machine, assuming happah-core and happah-graphics have already been installed:

```
sudo apt install libglfw3-dev
mkdir -p ${HOME}/Workspace/{bin,include,lib}
git clone http://github.com/happah-graphics/happah-viewer.git
cd happah-viewer
./bootstrap
./configure --prefix=${HOME}/Workspace
export CPATH="${HOME}/Workspace/include"
export LD_LIBRARY_PATH="${HOME}/Workspace/lib"
make install
ln -s ../happah-graphics/shaders
```

Make your changes and run ``` make ``` to compile the application and ``` make install ``` to install the application into the bin directory.  Finally, run ``` git push origin master ``` to upload your changes to Github.

If you have a release-ready version, tag it by executing ``` git tag -a v0.1 -m "version 0.1" ``` and upload the tag to Github using ``` git push origin v0.1 ``` to push a specific tag or ``` git push origin --tags ``` to push all tags at once.

To use the viewer, execute ``` happah path-to-off-file ```.

