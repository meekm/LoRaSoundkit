function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  
  var i = 0;  // nibbleCount (nibble is 4 bits)
  var decoded = {};
  
  // get 12 bits value from payload and convert it to float and divide by 10.0
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

// get array of values
  function getList12( len) {
    var list = [];
    for( j=0; j<len; j++)
      list[j] = getVal12();
    return list;
  }


// decode payload in spectrum peak and average for level a, c and z
 if (port === 10) {

    decoded.la = {};
    decoded.la.spectrum = getList12(9);   // list length = 9
    decoded.la.peak = getVal12();
    decoded.la.avg = getVal12();

    decoded.lc = {};
    decoded.lc.spectrum = getList12(9);   // list length = 9
    decoded.lc.peak = getVal12();
    decoded.lc.avg = getVal12();
    
    decoded.lz = {};    
    decoded.lz.spectrum = getList12(9);   // list length = 9
    decoded.lz.peak = getVal12();
    decoded.lz.avg = getVal12();
 }
 
  return decoded;
}