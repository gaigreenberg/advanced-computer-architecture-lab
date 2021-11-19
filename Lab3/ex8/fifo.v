module fifo(clk, reset, in, push, pop, out, full);
   parameter N=4; // determines the maximum number of words in queue.
   parameter M=2; // determines the bit-width of each word, stored in the queue.

   input clk, reset, push, pop;
   input [M-1:0] in;
   output [M-1:0] out;
   output full;

   // Fill Here

   integer        n = 0 ;
   reg            full_fifo = 0 ;
   reg [N*M-1:0]  fifo = 0;
   reg [M-1:0]    word ;

   always @(posedge clk or reset) begin
      if(reset) begin   //reset everything to make sure 
         n <= 0 ;
         fifo <= 0 ;
         word <= 0 ;
         full_fifo <= 0; 
      end
      else begin
         
         if ( n == 0) begin //fifo is empty 
            if ( push == 1) begin
               n <= 1 ;
               word <= in ; 
               fifo <= (in << M*(N-1)) | fifo ; 
            end
            else begin 
               if(~push) 
                  word <= 2'd0 ; 
            end
         end

         else begin // n != 0 

            if( n >= N) begin //should never be n>N but only to make sure 
               if(push == 1 && pop == 1) begin
                  word <= fifo;
                  fifo <= ( in << M*(N-1)) | (fifo >>> M );
               end else; 

               if (push == 0 && pop == 1) begin
                  n <= n -1 ;
                  full_fifo <= 0 ; 
                  word <= fifo ; 
                  fifo <= ((fifo >>> M) <<< M) ; 
               end else ;
            end 

            else begin // 0 < n < N case 
               if(push == 1 && pop == 1) begin
                  word <= (fifo >>> M*(N-n));
                  fifo <= ( in << M*(N-1)) | (((fifo >>> M*(N-n+1)) <<< M*(N-n+1)) >>> M);
               end else;

               if( push ==1 && pop == 0) begin 
                  word <= (fifo >>> M*(N-n));
                  fifo <= (in << (M*(N-1))) | fifo >>> M ;
                  n <= n + 1 ; 
                  if( n == N - 1 )
                     full_fifo <= 1 ;
               end else ;

               if( push ==0 && pop == 1 ) begin 
                  word <= (fifo >>> M*(N-n)) ;
						fifo <= ((fifo >>> M*(N-n+1)) <<< M*(N-n+1));
						n <= n - 1;
					end else;
            
            end
         end

      end
   end 
   assign out = word;
   assign full = full_fifo;


endmodule
