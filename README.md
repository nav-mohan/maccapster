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

