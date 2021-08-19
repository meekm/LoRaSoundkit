function decodeUplink(input) {

  // Payload decoder for Lora Soundkit.
  // Decode binary uplink message to json message
  // the payload byte buffer contains 19 bytes in the order:
  // byte 0: a constant (this constant is a multiply factor to correct byte values 1 upto 18)
  // byte 1-9: 9 bytes containg la.min, la.max, la.avg, lc.min, lc.max, lc.avg, lz.min, lz.max, lz.avg
  // byte 10-18: 9 bytes containing lz spectrum representing octaves from 31.5Hz to 8kHz
  // the payload formatter calculates from the lz spectrum the lc and la spectrum
  // the constant in byte 0 corrects the values in byte 1 upto 18
  // by Marcel Meek, May 2020
  
  // weigthing tables
  var aWeighting = [ -39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1 ];
  var cWeighting = [  -3.0,  -0.8,  -0.2,  0.0,  0.0, 0.0, 0.2, 0.3, -3.0 ];
  var len = aWeighting.length;

  var decoded = {};  // json result
  var bytes = input.bytes;
  var i = 0;
 
   // decode 19 bytes payload (new format)
  if (input.fPort === 22) {
    var max = bytes[i++];
    var c = max / 255.0;

    decoded.la = {};
    decoded.lc = {};
    decoded.lz = {};    
    
  // get min, max and avg for LA, LC and LZ
    decoded.la.min = c * bytes[i++];
    decoded.la.max = c * bytes[i++];
    decoded.la.avg = c * bytes[i++];    
    decoded.lc.min = c * bytes[i++];
    decoded.lc.max = c * bytes[i++];
    decoded.lc.avg = c * bytes[i++];    
    decoded.lz.min = c * bytes[i++];
    decoded.lz.max = c * bytes[i++];
    decoded.lz.avg = c * bytes[i++];
  
    // get LZ spectrum
    decoded.lz.spectrum = [];
    for( j=0; j<len; j++) 
      decoded.lz.spectrum[j] = c * bytes[i++];
   
    // convert LZ spectrum to LA
    decoded.la.spectrum = [];
    for( j=0; j<len; j++) 
      decoded.la.spectrum[j] = decoded.lz.spectrum[j] + aWeighting[j];

    // convert LZ spectrum to LC
    decoded.lc.spectrum = [];
    for( j=0; j<len; j++) 
      decoded.lc.spectrum[j] = decoded.lz.spectrum[j] + cWeighting[j];
  }

  return { data: decoded, warnings: [], errors: [] };
}
