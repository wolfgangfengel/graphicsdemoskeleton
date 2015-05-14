All the code in this repository tries to pack into a small exe. There are in the moment five examples:

  * Skeleton - this is the basic DX11 framework example and it compresses to < 720 bytes in size
  * DirectCompute Julia4D - this is a compute shader example that is a port of Jan Vlietnick's Julia 4D demo. After compression it is about 2,920 bytes.
  * DirectCompute PostFX Color Filters - this is a compute shader example that features a simple post-processing pipeline with color filters. After compression it is about 3,536 bytes.

The DirectCompute examples are used in my UCSD class to teach DirectCompute. All the examples do not have proper error checking and are generally dangerous for your computer :-) ... use at your own risk.

What you need is
  * Visual Studio Express 2013 for Windows Desktop because of the partial C99 support
  * Crinkler 1.4 is used as an exe compressor (comes in the repository)


References:
  * Here is an article that describes how to reduce the size of a exe with C/C++
http://www.catch22.net/tuts/reducing-executable-size
  * There is also a whole DirectX 9.1 / OpenGL demo framework released by Inigo Quelez here
http://www.iquilezles.org/www/material/isystem1k4k/isystem1k4k.htm
  * A blog on demo development and there is a link to a demo framework as well:
http://yupferris.blogspot.com/
  * 4klang is a modular software synthesizer package intended to easily produce music for 4k intros:
http://4klang.untergrund.net/
  * If you want to use asm:
http://japheth.de/h2incX.html
  * Shader Minifier
http://www.ctrl-alt-test.fr/?page_id=7

Please send me more references that I can post here ...