function Decoder(bytes, port) {

  // Payload decoder for Lora Soundkit.
  // Decode binary uplink message to json message
  // the byte buffer contains 18 sound values in the order:
  // la.min, la.max, la.avg, lc.min, lc.max, lc.avg, lz.min, lz.max, lz.avg, lz.spectrum (9 values).
  // each binary value is 12 bits
  // the la and lc spectrum is calculated from lz spectrum
  // by Marcel Meek, May 2020
  
  // weigthing tables
  var aWeighting = [ -39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1 ];
  var cWeighting = [  -3.0,  -0.8,  -0.2,  0.0,  0.0, 0.0, 0.2, 0.3, -3.0 ];

  var i = 0;         // nibbleCount (nibble is 4 bits)
  var decoded = {};  // json result
  
  // get a 12 bit value from payload buffer and convert it to float and divide by 10.0
  function getVal12() {
    var val = 0;
     if( i % 2 === 0) {
        val = bytes[i>>1] << 4;
        val |= (bytes[(i>>1) +1] & 0xf0) >> 4
     }
     else {
         val = (bytes[i>>1]  & 0x0f) << 8;
         val |= (bytes[(i>>1) +1]);
     }
      // test sign bit (msb in a 12 bit value)
     if( val & 0x800) 
       val |= 0xfffff000;     // make negative
     i += 3;
     return val / 10.0;
  }

 // get list of 12 bit values from payload buffer 
  function getList12( len) {
    var list = [];
    for( j=0; j<len; j++)
      list[j] = getVal12();
    return list;
  }

// decode payload
  if (port === 21) {

    var len = aWeighting.length;

    decoded.la = {};
    decoded.la.min = getVal12();
    decoded.la.max = getVal12();
    decoded.la.avg = getVal12();
 
    decoded.lc = {};
    decoded.lc.min = getVal12();
    decoded.lc.max = getVal12();
    decoded.lc.avg = getVal12();
    
    decoded.lz = {};    
    decoded.lz.min = getVal12();
    decoded.lz.max = getVal12();
    decoded.lz.avg = getVal12();
    
    decoded.lz.spectrum = getList12(len);
    
    // convert LZ spectrum to LA spectrum
    decoded.la.spectrum = [];
    for( i=0; i<len; i++) 
      decoded.la.spectrum[i] = decoded.lz.spectrum[i] + aWeighting[i];
 
    // convert LZ spectrum to LC spectrum
    decoded.lc.spectrum = [];
    for( i=0; i<len; i++) 
      decoded.lc.spectrum[i] = decoded.lz.spectrum[i] + cWeighting[i];
    
  }
  return decoded;
}
