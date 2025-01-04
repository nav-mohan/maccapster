## MacCapster
`MacCapster` is a [Common-Alert Protocol](https://en.wikipedia.org/wiki/Common_Alerting_Protocol) compliant client for Mac-OS that supports Text-to-Speech conversion, SPDT relay controls, audio playback, and `MP3`/`AAC` compression at standard bitrates and sample-rates. 

The goal of this project is to help small independant radio stations stay compliant with FCC and CRTC regulations.

## Dependancies
* [Boost::ASIO](https://www.boost.org/doc/libs/1_87_0/doc/html/boost_asio.html) and [OpenSSL](https://www.openssl.org/) for encrypted client-server communication.
* [Boost::PropertyTree](https://www.boost.org/doc/libs/1_87_0/doc/html/property_tree.html) for XML-parsing.
* [AVFoundation Framework](https://developer.apple.com/av-foundation/) for Text-to-Speech conversion.
* [AudioToolBox Framework](https://developer.apple.com/documentation/audiotoolbox?language=objc) for audio playback.
* [Foundation](https://developer.apple.com/documentation/foundation?language=objc) and [Core Foundation](https://developer.apple.com/documentation/corefoundation?language=objc) for data-types. 
* [LibMP3LAME](https://lame.sourceforge.io) and [FDK-AAC](https://github.com/mstorsjo/fdk-aac) for MP3/AAC compression.
* [MSLogger](https://github.com/nav-mohan/mslogger) for thread-safe logging.

## Design
Being a real-time application, `MacCapster` makes extensive use of asynchronous and multithreaded design paradigms. It begins with listening for incoming emergency alerts from an authorized source. Upon receiving an alert, the software performs collision detection for geodetic data, ensuring the alert's geographical area does not overlap with existing warnings. Once validated, the system uses the Mac-OS native text-to-speech technology to convert the alert message into a Mono `.WAV` file. Upon completion of the TTS, `MacCapster` triggers a Single Pole Double Throw (SPDT) relay control through a [`USB` to `RS232`](https://en.wikipedia.org/wiki/USB-to-serial_adapter) adapter to switch the broadcast signal from regular programming to the emergency alert. Then, the alert message is played back through the radio station’s audio system, broadcasting the warning to the public before flipping the SPDT back to resume the regular broadcast. Finally, the `.WAV` files are encoded to `MP3`/`AAC` to reduce storage. 

### Future work
* Implement an asynchronous archiving pipeline to archive old `XML`, `MP3`/`AAC` files and reduce storage. 
This will be built using [`libarchive`](https://github.com/nav-mohan/example_libarchive/).

### References/Notes
* [CAP Wikipedia](https://en.wikipedia.org/wiki/Common_Alerting_Protocol)
* [CAP United Nations](https://www.undrr.org/early-warnings-for-all/common-alerting-protocol) 
* [CAP Canadian Profile](https://www.publicsafety.gc.ca/cnt/mrgnc-mngmnt/mrgnc-prprdnss/capcp/index-en.aspx)
* [CAP Schema](http://docs.oasis-open.org/emergency/cap/v1.2/CAP-v1.2.xsd)
* [CAP Documentation](http://docs.oasis-open.org/emergency/cap/v1.2/CAP-v1.2.xsd)
* [CAP Documentation from Pelmorex](https://alerts.pelmorex.com/wp-content/uploads/2020/09/NAADS-LMD-User-Guide-R10.0.pdf)