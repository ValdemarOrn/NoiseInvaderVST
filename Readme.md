# Introduction

It came to my attention that there is a serious lack of good noise gates available for free. To address this need, I decided to design this noise gate algorithm and release it as a free (as in beer and speech) VST plugin to the world. I hope you will find it as useful as I do. 

## Download

Get the latest download from the **[Release page](www.google.com)**.

## License

Am am releasing the code in this repository in to the public domain. Do with it whatever you please.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## How it works

The manual gives an in-depth explanation of the how the algorithm works. **[You can read the manual here](www.google.com)**.

## Building from Source

In order to build from source you must obtain a copy of the VST SDK from Steinberg, as they do not allow the SDK to be distributed by 3rd parties. This plugin uses VST 2.4, which is included as part of the most recent (version 3) SDK.

Once you've obtained the SDK, modify the include directories to point to the location of the SDK. Finally, one minor change has to be done to the vstpluginmain.cpp file.

Replace the following line:

    #define VST_EXPORT

with

    #define VST_EXPORT __declspec(dllexport)

This will export the vstmain function so that it is available as a function in your dll file. If you don't do this change, you can also specify a list of functions to be exported in VST, but I prefer to use this way instead. 