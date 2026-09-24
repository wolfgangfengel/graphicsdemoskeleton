// Build-time tool only. The demo itself uses the small C decoder.
#include "smolv.h"
#include <cstdio>
#include <cstring>
#include "../../Vulkan/Common/Smolv.h"
int main(int argc,char **argv)
{
    if(argc!=3) return 1;
    FILE *file;
    if(fopen_s(&file,argv[1],"rb")) return 2;
    fseek(file,0,SEEK_END); long length=ftell(file); rewind(file);
    if(length<=0) { fclose(file); return 3; }
    smolv::ByteArray input((size_t)length),encoded,decoded((size_t)length),tiny((size_t)length);
    bool read=fread(input.data(),1,input.size(),file)==input.size(); fclose(file);
    if(!read || !smolv::Encode(input.data(),input.size(),encoded,0)) return 4;
    if(!smolv::Decode(encoded.data(),encoded.size(),decoded.data(),decoded.size()) || input!=decoded) return 5;
    // Validate the actual runtime C decoder too, for every generated shader.
    if(!smolDecode(encoded.data(),(unsigned int)encoded.size(),tiny.data(),(unsigned int)tiny.size()) || input!=tiny) return 6;
    if(fopen_s(&file,argv[2],"wb")) return 7;
    bool wrote=fwrite(encoded.data(),1,encoded.size(),file)==encoded.size(); fclose(file);
    if(!wrote) return 8;
    printf("SMOL-V: %u -> %u bytes; both decoders restore identical SPIR-V.\n",(unsigned int)input.size(),(unsigned int)encoded.size());
    return 0;
}
