import numpy as np
import struct

import io
import sqlite3
#from bitstring import BitArray

def importdb(db):
     conn = sqlite3.connect(db)
     c = conn.cursor()
     c.execute("SELECT name FROM sqlite_master WHERE type='table';")
     results = c.fetchall()
     for table in results:
         print("table: ", table)
         yield list( c.execute("SELECT * from "+table[0]+";") )
#         c.execute("SELECT * FROM ?;", (table[0],))
#         tmp = c.fetchall()
#         print(tmp)

     # disconnect from server
     c.close()
     conn.close()



BIAS = 1023                # constant to be subtracted from the stored exponent
SIGN_BIT = 63              # bit index at which the sign is stored
EXP_BIT = 52               # bit index at which the exponent starts
EXP_MASK = 0x000fffffffffffff  # bit-mask to remove exponent, leaving mantissa
NAN_EXP = 0x7ff            # special exponent which encodes NaN/Infinity

def parse_hex(hexstring, float_format='%.15e', no_decimal=False):
    """ Take a 8-byte hex string (16 digits) representing a double precision,
        parse it, and print detailed information about the represented float
        value.
    """
    bits = int('0x%s' % hexstring, 32)
#    bits = int('0x%s' % hexstring, 64)
    sign = '+1'
    if test_bit(bits, SIGN_BIT) > 0:
        sign = '-1'
    bits = clear_bit(bits, SIGN_BIT)
    stored_exp = bits >> EXP_BIT
    mantissa = bits & EXP_MASK # mask the exponent bits

    print ("")
    print ("Bytes         = 0x%s" % hexstring)
    print ("Float         = "+ float_format \
                           % struct.unpack('!d', hexstring.decode('hex'))[0])
    print ("Sign          = %s" % sign)
    if stored_exp == 0:
        print ("Exponent      = 0x%x (Special: Zero/Subnormal)" % stored_exp)
        print ("Mantissa      = 0x%x" % mantissa)
        if not no_decimal:
            if mantissa == 0:
                print ("Exact Decimal = %s0" % sign[0])
            else:
                print ("Exact Decimal = %s (subnormal)" \
                    % float2decimal(hex2float(hexstring)))
    elif stored_exp == NAN_EXP:
        print ("Exponent      = 0x%x (Special: NaN/Infinity)" % stored_exp)
        print ("Mantissa      = 0x%x" % mantissa)
        if not no_decimal:
            if mantissa == 0:
                print ("Exact Decimal = %sInfinity" % sign[0])
            else:
                print ("Exact Decimal = NaN")
    else:
        print ("Exponent      = 0x%x = %i (bias %i)" % (stored_exp,
                                                       stored_exp, BIAS))
        print ("Mantissa      = 0x%x" % mantissa)
        if not no_decimal:
            mantissa = set_bit(mantissa, EXP_BIT) # set the implicit bit
            print ("Exact Decimal = %s 2^(%i) * [0x%x * 2^(-52)]" \
                                % (sign[0], stored_exp-BIAS, mantissa))
            print ("              = %s" % float2decimal(hex2float(hexstring)))

def main():
#    string = np.array([500],dtype=np.float64)
    string = np.array([ [1,2,3],[4,5,6] ], dtype=np.float32)
    print(string[0,0])
    print(string[0,2])
    string[0,2] = 9
    # string with encoding 'utf-8'
    arr = bytes(string)
    # parse_hex(str(arr)[2:-1])
    #bArr = BitArray(arr)
    #print(struct.unpack('f', arr[:8]))
    #print(int(arr,2))
    print(type(arr))
    print(string.shape)
    print(str(arr))
    print(len(arr))

    print(arr[0],arr[1],arr[2],arr[3],arr[4],arr[5],arr[6],arr[7])
    print(struct.unpack_from('f', arr[0:4]))
    print(struct.unpack_from('f', arr[4:8]))
    print(struct.unpack_from('f', arr[8:12]))
    print(struct.unpack_from('f', arr[12:16]))
    print(struct.unpack_from('f', arr[16:20]))
    print(struct.unpack_from('f', arr[20:24]))

    print("start")
    db = "database_originalSouthBuilding.db"
    print("import from", db)
    importdb(db)

    for x in range(3):
        print(x)
# conn = sqlite3.connect(db)
# c = conn.cursor()
# c.execute("SELECT name FROM sqlite_master WHERE type='table';")
# results = c.fetchall()
# print(results)
# for table in results:
#     curTmp = conn.cursor()
#     print("table: ", table[0])
#     print("table.dtype: ", type(table[0]))
#     SQLquery = "SELECT * FROM "+table[0]+";"
# #    yield list( c.execute(SQLquery) )
#     curTmp.execute(SQLquery)
#     tmp = curTmp.fetchall()
#     print(tmp)
#     curTmp.close()
# # disconnect from server
# c.close()
# conn.close()

if __name__ == "__main__":
    main()
