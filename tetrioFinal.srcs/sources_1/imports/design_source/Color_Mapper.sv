//-------------------------------------------------------------------------
//    Color_Mapper.sv                                                    --
//    Stephen Kempf                                                      --
//    3-1-06                                                             --
//                                                                       --
//    Modified by David Kesler  07-16-2008                               --
//    Translated by Joe Meng    07-07-2013                               --
//    Modified by Zuofu Cheng   08-19-2023                               --
//                                                                       --
//    Fall 2023 Distribution                                             --
//                                                                       --
//    For use with ECE 385 USB + HDMI                                    --
//    University of Illinois ECE Department                              --
//-------------------------------------------------------------------------
module color_mapper ( input logic [9:0]DrawX, DrawY, 
        input logic[7:0] data, 
        input logic [31:0] memdata, 
        input logic [31:0] ctrlreg, 
        output logic [3:0] Red, Green, Blue, 
        output logic [10:0] fontselect, 
        output logic[9:0] vramaddr ); 
        logic bckgrnd; 
        logic [6:0] col ; // which row and column the pixel we're drawing is in 
        logic [4:0] row ; 
        
        assign col = DrawX >> 3; 
        assign row = DrawY >> 4; 
        assign vramaddr = (col >> 2) + (row * 5'd20); 
        
        logic [1:0] col_byte_select; 
        assign col_byte_select = col[1:0]; 
        logic [7:0] bytedata; 
        assign fontselect = (bytedata[6:0] * 7'd16) + DrawY[3:0]; 
        logic[2:0] pxl; 
        assign pxl = 7 - DrawX[2:0]; 
        assign bckgrnd = ((bytedata[7] & data[pxl]) |(~bytedata[7] & ~data[pxl])); 
        
        always_comb begin 
            case(col_byte_select) 
                2'd0: bytedata = memdata[7:0]; 
                2'd1: bytedata = memdata[15:8]; 
                2'd2: bytedata = memdata[23:16]; 
                2'd3: bytedata = memdata[31:24]; 
            endcase 
        end 
        always_comb begin:RGB_Display 
            if ((bckgrnd == 1'b1)) begin 
                Red = ctrlreg[11:8]; 
                Green = ctrlreg[7:4]; 
                Blue = ctrlreg[3:0]; 
                end 
            else begin 
                Red = ctrlreg[27:24]; 
                Green = ctrlreg[23:20]; 
                Blue = ctrlreg[19:16]; 
            end 
        end 
endmodule




