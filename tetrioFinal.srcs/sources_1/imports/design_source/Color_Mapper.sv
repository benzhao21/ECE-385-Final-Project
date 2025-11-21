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
        
        
endmodule




