/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied.  See the License for the specific language governing
  permissions and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  simple_fifo.cpp -- Simple SystemC 2.0 producer/consumer example.

                     From "An Introduction to System Level Modeling in
                     SystemC 2.0". By Stuart Swan, Cadence Design Systems.
                     Available at www.accellera.org

  Original Author: Stuart Swan, Cadence Design Systems, 2001-06-18

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/


#include <systemc.h>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>


using namespace std;


class write_if : virtual public sc_interface
{
   public:
     virtual void write(char) = 0;
     virtual void reset() = 0;
};

class read_if : virtual public sc_interface
{
   public:
     virtual void read(char &) = 0;
};

class fifo : public sc_channel, public write_if, public read_if
{
   public:
    sc_port<sc_signal_out_if<bool> > write_available;
    sc_port<sc_signal_out_if<bool> > read_available;
    sc_port<sc_signal_in_if<bool> > END;

    fifo(sc_module_name name, int buffer_size) : sc_channel(name){
        addr = 0;
        num_element = 0;
        BUFFER_SIZE = buffer_size;
        buffer = (char*)malloc(sizeof(char)*buffer_size);

        for(int i = 0; i < buffer_size; i++){
            buffer[i] = '\0';
        }

    }


     void write(char c) {

         buffer[addr] = c;

         addr++;

         if(addr == BUFFER_SIZE || END->read()){
            write_available->write(false);
            read_available->write(true);
            num_element = addr;
         }

     }

     void read(char &c){

         c = buffer[num_element - addr];

         addr--;

         if(addr == 0){
            read_available->write(false);
            write_available->write(true);
         }
     }

     void reset() {
            addr = 0;
     }


   private:
     int BUFFER_SIZE;
     char *buffer;
     int addr;
     int num_element;
};

class producer : public sc_module
{
   public:
     sc_in<bool> clk;

     sc_port<sc_signal_in_if<bool> > A_write_available;
     sc_port<sc_signal_in_if<bool> > B_write_available;
     sc_port<sc_signal_out_if<bool> > END;

     sc_port<write_if> outA;
     sc_port<write_if> outB;


     SC_HAS_PROCESS(producer);

     const char *str  = "Visit www.accelera.org and see what SystemC can do for you today!\n";

     producer(sc_module_name name, int buffer_size) : sc_module(name)
     {
       SC_CTHREAD(write, clk.pos());
       addr = 0;
       num_element = 0;
       ping = true;
       BUFFER_SIZE = buffer_size;
     }

     void write()
     {
       while(*(str + addr)){
               wait();
               if(ping && A_write_available->read()){

                   outA->write(*(str + addr));

                   cout << "\033[1;31m"
                        << "Write to  buffer A    @ "
                        << sc_time_stamp()
                        << "\033[0m"
                        << endl;

                   addr++;

                   if(((++num_element)%BUFFER_SIZE)==0){
                    ping = !ping;
                   }

               }
               else if(!ping && B_write_available->read()){
                   outB->write(*(str + addr));

                   cout << "\033[1;32m"
                        << "Write to  buffer B    @ "
                        << sc_time_stamp()
                        << "\033[0m"
                        << endl;

                   addr++;

                   if(((++num_element)%BUFFER_SIZE)==0){
                    ping = !ping;
                   }
               }
               if(*(str+addr) == 10){
                   END->write(true);
               }

        }
     }



   private:
     int addr;
     int num_element;
     bool ping;
     int BUFFER_SIZE;

};

class consumer : public sc_module
{
   public:
     sc_in<bool> clk;

     sc_port<read_if> inA;
     sc_port<read_if> inB;

     sc_port<sc_signal_in_if<bool> > A_read_available;
     sc_port<sc_signal_in_if<bool> > B_read_available;
     sc_port<sc_signal_in_if<bool> > END;

     SC_HAS_PROCESS(consumer);

     consumer(sc_module_name name, int buffer_size) : sc_module(name)
     {
       SC_CTHREAD(read, clk.pos());
       num_element = 0;
       pong = true;
       BUFFER_SIZE = buffer_size;
     }

     void read()
     {
       char c;

       while (true) {

         wait();



         if(pong && A_read_available->read()){

             num_element = (num_element + 1) % BUFFER_SIZE;

             inA->read(c);
             cout   << "\033[1;35m"
                    << "Read from buffer A: "
                    << c
                    << " @ "
                    << sc_time_stamp()
                    << "\033[0m"
                    << endl;
             if( num_element == 0){
                pong = !pong;
             }
         }
         else if(!pong && B_read_available->read()){

             num_element = (num_element + 1) % BUFFER_SIZE;

             inB->read(c);
             cout   << "\033[1;36m"
                    << "Read from buffer B: "
                    << c
                    << " @ "
                    << sc_time_stamp()
                    << "\033[0m"
                    << endl;
             if( num_element == 0){
                pong = !pong;
             }
         }
         // if(END->read() && (!A_read_available->read() && !B_read_available->read()))
         // {
            // cout << "Stop @" <<sc_time_stamp() << endl;
            // sc_stop();
         // }
       }
     }

   private:
     bool pong;
     int num_element;
     int BUFFER_SIZE;
};


class top : public sc_module
{
   public:
     producer *prod_inst;
     consumer *cums_inst;

     fifo *buffer_A;
     fifo *buffer_B;

     sc_signal<bool, SC_MANY_WRITERS> A_write_available;
     sc_signal<bool, SC_MANY_WRITERS> A_read_available;
     sc_signal<bool, SC_MANY_WRITERS> B_write_available;
     sc_signal<bool, SC_MANY_WRITERS> B_read_available;
     sc_signal<bool, SC_MANY_WRITERS> END;

     top(sc_module_name name,int buffer_size) : sc_module(name),
     A_write_available("AWA"),
     A_read_available("ARA"),
     B_write_available("BWA"),
     B_read_available("BRA"),
     END("END")
     {

       prod_inst = new producer("Producer", buffer_size);
       cums_inst = new consumer("Consumer", buffer_size);

       buffer_A = new fifo("BufferA", buffer_size);
       buffer_B = new fifo("BufferB", buffer_size);

       A_write_available.write(true);
       A_read_available.write(false);
       B_write_available.write(true);
       B_read_available.write(false);
       END.write(false);

       buffer_A->write_available(A_write_available);
       buffer_A->read_available(A_read_available);
       buffer_B->write_available(B_write_available);
       buffer_B->read_available(B_read_available);
       buffer_A->END(END);
       buffer_B->END(END);

       prod_inst->outA(*buffer_A);
       prod_inst->A_write_available(A_write_available);
       prod_inst->outB(*buffer_B);
       prod_inst->B_write_available(B_write_available);
       prod_inst->END(END);

       cums_inst->inA(*buffer_A);
       cums_inst->A_read_available(A_read_available);
       cums_inst->inB(*buffer_B);
       cums_inst->B_read_available(B_read_available);
       cums_inst->END(END);

     }
};

int sc_main (int argc, char* argv[]) {
   int buffer_size = 10;

   if(argc > 1){
    buffer_size = atoi(argv[1]);
   }

   top top1("Top1", buffer_size);

   sc_clock p_clk("producer_clk", 1, SC_SEC, 0.5, 2, SC_SEC, true);
   sc_clock c_clk("consumer_clk", 2, SC_SEC, 0.5, 2, SC_SEC, true);


   top1.prod_inst->clk(p_clk);
   top1.cums_inst->clk(c_clk);


   sc_start(300, SC_SEC);
   return 0;
}
