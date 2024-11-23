#include "imgui.h"

#include "lib/ImGuiFileDialog/ImGuiFileDialog.h"
#include "lib/implot/implot.h"

// System includes
#include <ctype.h>          // toupper
#include <limits.h>         // INT_MIN, INT_MAX
#include <math.h>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>          // vsnprintf, sscanf, printf
#include <stdlib.h>         // NULL, malloc, free, atoi
#include <stdint.h>         // intptr_t

#include <algorithm>

#include <iostream>
#include <filesystem>

#include "lib/implot/implot.h"

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

//other files
namespace BrighterShoresXP {

    struct map_region {
        long start;
        long end;
    };

    static int brighter_shores_pid = 0;
    static std::list<map_region> maps;
    static int exp_memory_offset = 0;//0x27428cc;

    static bool CheckProcessUnix( int pid ){
        std::string path = "/proc/" + std::to_string(pid);
        if( std::filesystem::is_directory( std::filesystem::path( path ) ) ){
            path += "/comm";
            std::ifstream comm_file;
            comm_file.open( path );

            std::stringstream str_stream;
            str_stream << comm_file.rdbuf();
            std::string exe_name = str_stream.str();

            if( exe_name.find("Brighter Shores") != std::string::npos ) {
                return true;
            }
            return false;
        } else {
           return false;
        }
        return false;
    }

    static bool CheckProcess(){
        bool found = true;
        //Linux
        if ( !CheckProcessUnix( brighter_shores_pid ) ){ //current process pid is not brighter shores
            maps.clear();
            found = false;
            std::string proc = "/proc/";
            for (const auto & entry : std::filesystem::directory_iterator(proc)){
                std::string pid_str = entry.path().filename();
                try { 
                    int pid = std::stoi( pid_str );
                    if( CheckProcessUnix( pid ) ){
                        brighter_shores_pid =  pid;
                        std::cout << "Found pid: " << pid << std::endl;
                        found = true;
                    }
                } catch( std::exception& e ) {
                    //some files in proc aren't numbers
                };
            }   
        }
        //Windows
        //todo
        return found;
    }

    static void LoadMapsUnix(){
        std::string map_path = "/proc/" + std::to_string(brighter_shores_pid) + "/maps";
        std::ifstream map_file(map_path);
        std::string line;
        while( std::getline(map_file, line) ){
            int idx = 0;
            std::string map[3];
            for( char& c : line ){
                if( (c == '-' && idx == 0) || c == ' ' ){
                    idx += 1;
                } else {
                    map[idx] += c;
                }
                if( idx >= 3 ) break;
            }
            if( map[2][0] == 'r' && map[2][1] == 'w' ){ //only need to map regions that can be read / write
                map_region region;
                region.start = std::stol( "0x"+map[0], 0, 16 );
                region.end = std::stol( "0x"+map[1], 0, 16 );
                maps.push_back( region );
            }
        }
    }

    struct CurrentXP
    {
        //episode 1
        uint32_t guard;
        uint32_t chef;
        uint32_t fisherperson;
        uint32_t forager;
        uint32_t alchemist;
        //episode 2
        uint32_t scout;
        uint32_t gatherer;
        uint32_t woodcutter;
        uint32_t carpenter;
        //episode 3
        uint32_t minefighter;
        uint32_t bonewright;
        uint32_t miner;
        uint32_t blacksmith;
        uint32_t stonemason;
        //episode 4
        uint32_t watchperson;
        uint32_t detective;
        uint32_t leatherworker;
        uint32_t merchant;
    };
    static CurrentXP current_xp;

    static unsigned char pattern[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x08, 0x00, 0x0F, 0x00, 0x75, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFF, 0x1D, 0x00, 0x00, 0x00};
    static unsigned char pattern2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x46, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFF, 0x1D, 0x00, 0x00, 0x00};
    static bool check_pattern( unsigned char* input ){
        for( int i=0; i< sizeof(pattern); i++){
            if( input[i] != pattern[i] && input[i] != pattern2[i] && pattern[i] != 0x0F ) return false;
        }
        return true;
    }

    static void read_mem_into_struct(){
        std::string mem_path = "/proc/" + std::to_string(brighter_shores_pid) + "/mem";
        std::ifstream mem_file(mem_path);
        mem_file.seekg( exp_memory_offset );
        mem_file.read((char*)&current_xp, sizeof(CurrentXP));
    }

    static bool CheckAddress(){
        if( maps.size() == 0 ){
            //Linux
            LoadMapsUnix();
        }
        if( maps.size() == 0 ){ //something went wrong when loading maps
            return false;
        }

        std::string mem_path = "/proc/" + std::to_string(brighter_shores_pid) + "/mem";
        std::ifstream mem_file(mem_path);
        for( map_region region : maps ){
            if( exp_memory_offset >= region.start+sizeof(pattern) && exp_memory_offset < region.end ){ //current memory is valid
                mem_file.seekg( exp_memory_offset-sizeof(pattern) );
                char input[sizeof(pattern)];
                mem_file.read( input, sizeof(pattern) );
                if( check_pattern( (unsigned char*) input ) ){
                    return true;
                }
            }
        }
        //if you're here the current memory address is wrong but mem is loaded so search for the pattern
        for( map_region region : maps ){
            for( long idx = region.start; idx < region.end-4096; idx += 4096 ){

                //if( idx < 0x274295c-8192 ) continue;
                //if( idx > 0x274295c ) continue;

                mem_file.seekg(idx);
                int length = 4096;
                if( length > region.end-idx) length = region.end-idx;

                unsigned char input[ length ];
                mem_file.read( (char*) input, length ); 

                for( int i=0; i<length; i++){
                    if( input[i] == 0x75 || input[i] == 0x46 ){
                        int idx_pat = (idx+i-18);
                        //std::cout << "potential match: 0x" << std::hex << idx_pat << std::endl;
                        mem_file.seekg( idx_pat );
                        unsigned char pat[ sizeof(pattern) ];
                        mem_file.read( (char*) pat, sizeof(pattern) );
                        if( check_pattern( pat ) ){
                            exp_memory_offset = idx_pat + sizeof(pattern);
                            return true;
                        }
                    }
                }
            }


        }

        return false;
    }

    struct Profession {
        bool reset = true;
        time_t start_time;
        time_t last_update;
        uint32_t start_exp = 0;
        double xp_hour = 0;

        uint32_t target_level = 0;

        int data_points = 0;
        uint32_t exp_history[1440]; // 1 per minute = 24hrs
        double exp_hour_history[1440]; // 1 per minute = 24hrs

        void Update(int current_exp){
            if( reset ){
                start_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
                start_exp = current_exp;
                last_update = start_time;
                for(int i = 0; i < 1440; i++){
                     exp_history[i] = start_exp;
                     exp_hour_history[i] = 0.0;
                }
                reset = false;
                data_points = 1;
            }
            time_t current_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
            time_t elapsed = current_time - start_time;
            double hours = elapsed / 3600.0;
            uint32_t xp_diff = current_exp-start_exp;
            if( hours > 0 ){
                xp_hour = xp_diff / hours;
            }

            if( current_time - last_update > 60){
                last_update += 60;
                for(int i = 0; i < 1439; i++){
                    exp_history[i] = exp_history[i+1];
                    exp_hour_history[i] = exp_hour_history[i+1];
                }
                exp_history[1439] = current_exp;
                exp_hour_history[1439] = xp_hour;
                data_points += 1;
                if (data_points >= 1440 ) data_points = 1440;
            }
        }

        void Draw(int current_exp, const char* title ){
            ImGui::SeparatorText( title );
            ImGui::Text( "Gained %d",  current_exp - start_exp );
            ImGui::Text( "XP/h %.2f",  xp_hour );

            static ImPlotAxisFlags xflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight;
            static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight;

            /*if( ImPlot::BeginPlot("XP", ImVec2(-1,64), ImPlotFlags_::ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame) ){

                //ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
                ImPlot::SetupAxes("Minutes","XP",xflags,yflags);
                ImPlot::PlotLine("Total", &exp_history[1440-data_points], data_points);
               
                ImPlot::EndPlot();
            }*/
            if( ImPlot::BeginPlot("XP/h", ImVec2(-1,64), ImPlotFlags_::ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame) ){

                //ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
                ImPlot::SetupAxes("Minutes","XP/h",xflags,yflags);
                ImPlot::PlotLine("Xp/H", &exp_hour_history[1440-data_points], data_points);
                ImPlot::EndPlot();
            }
            ImGui::PushID( title );
            if( ImGui::Button("Reset") ){
                std::cout << "?" << std::endl;
                reset = true;
            }
            ImGui::PopID();
        }
             
    };

    static void RenderGUI(){

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        if( !CheckProcess() ){
            ImGui::Begin("Can't find Brighter Shores process...");
            ImGui::End();
            return;
        }

        if( !CheckAddress() ){
            ImGui::Begin("Can't find memory offsets...");
            ImGui::End();
            return;
        }

        read_mem_into_struct();        

        //Ep1
        static Profession guard;
        ImGui::Begin("Ep1");
        guard.Update( current_xp.guard );
        guard.Draw( current_xp.guard, "Guard" );

        static Profession chef;
        chef.Update( current_xp.chef );
        chef.Draw( current_xp.chef, "Chef"  );       

        static Profession fisherperson;
        fisherperson.Update( current_xp.fisherperson );
        fisherperson.Draw( current_xp.fisherperson, "Fisherperson"  );

        static Profession forager;
        forager.Update( current_xp.forager );
        forager.Draw( current_xp.forager, "Forager"  );

        static Profession alchemist;
        alchemist.Update( current_xp.alchemist );
        alchemist.Draw( current_xp.alchemist, "Alchemist"  );
        ImGui::End();  

        //Ep2
        static Profession scout;
        ImGui::Begin("Ep2");
        scout.Update( current_xp.scout );
        scout.Draw( current_xp.scout, "Scout"  );

        static Profession gatherer;
        gatherer.Update( current_xp.gatherer );
        gatherer.Draw( current_xp.gatherer, "Gatherer"  );      

        static Profession woodcutter;
        woodcutter.Update( current_xp.woodcutter );
        woodcutter.Draw( current_xp.woodcutter, "Woodcutter"  ); 

        static Profession carpenter;
        carpenter.Update( current_xp.carpenter );
        carpenter.Draw( current_xp.carpenter, "Carpenter"  );
        ImGui::End();   

        //Ep3
        static Profession minefighter;
        ImGui::Begin("Ep3");
        minefighter.Update( current_xp.minefighter );
        minefighter.Draw( current_xp.minefighter, "Minefighter"  );

        static Profession bonewright;
        bonewright.Update( current_xp.bonewright );
        bonewright.Draw( current_xp.bonewright, "Bonewright"  );

        static Profession miner;
        miner.Update( current_xp.miner );
        miner.Draw( current_xp.miner, "Miner" ); 

        static Profession blacksmith;
        blacksmith.Update( current_xp.blacksmith );
        blacksmith.Draw( current_xp.blacksmith, "Blacksmith"  );

        static Profession stonemason;
        stonemason.Update( current_xp.stonemason );
        stonemason.Draw( current_xp.stonemason, "Stonemason"  );
        ImGui::End();

        //Ep4
        static Profession watchperson;
        ImGui::Begin("Ep4");
        watchperson.Update( current_xp.watchperson );
        watchperson.Draw( current_xp.watchperson, "Watchperson"  );

        static Profession detective;
        detective.Update( current_xp.detective );
        detective.Draw( current_xp.detective, "Detective"  );      

        static Profession leatherworker;
        leatherworker.Update( current_xp.leatherworker );
        leatherworker.Draw( current_xp.leatherworker, "Leatherworker"  );

        static Profession merchant;
        merchant.Update( current_xp.merchant );
        merchant.Draw( current_xp.merchant, "Merchant"  );
        ImGui::End();
    }
}