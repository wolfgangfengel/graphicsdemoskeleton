//
// Blit a structured buffer to the screen
//
StructuredBuffer<float> buffer : register( t0 );

struct PsIn
{
    float4 Pos : SV_POSITION;
};

#define WINDOWWIDTH 800
#define WINDOWHEIGHT 640

PsIn VSBlit(uint VertexID: SV_VertexID)
{
	PsIn Out = (PsIn)0;

	// Produce a fullscreen triangle with a triangle list
	float4 position;
	position.x = (VertexID == 2)?  3.0 : -1.0;
	position.y = (VertexID == 0)? -3.0 :  1.0;
	position.zw = 1.0;

	Out.Pos = position;

	return Out;
}

// unpack three positive normalized values from a 32-bit float
float4 Unpack4PNFromFP32(float fFloatFromFP32)
{
 float r, g, b, d;
 uint uValue;
 
 uint uInputFloat = asuint(fFloatFromFP32);
 
 // unpack a
 // mask out all the stuff above 16-bit with 0xFFFF
 //a = ((uInputFloat) & 0xFFFF) / 65535.0;
 r = ((uInputFloat) & 0xFF) / 255.0;
 
 g = ((uInputFloat >> 8) & 0xFF) / 255.0;
  
 b = ((uInputFloat >> 16) & 0xFF) / 255.0;
 
 // extract the 1..254 value range and subtract 1
 // ending up with 0..253
 d = (((uInputFloat >> 24) & 0xFF) - 1.0) / 253.0;

 return float4(r, g, b, d);
}


float4 PSBlit(PsIn In) : SV_TARGET
{
   uint idx = ((In.Pos.x)) + ((In.Pos.y)) * WINDOWWIDTH;  

   return Unpack4PNFromFP32(buffer[idx]);	
}
