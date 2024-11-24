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
    static const int exp_table[] = {500,1015,1545,2090,2651,3229,4418,5642,6902,8199,9534,10908,13736,16647,19644,22728,25903,29171,32535,35997,53815,71979,90496,109373,128617,148236,168236,188625,209410,230599,252200,274221,296670,319556,342887,366671,390918,415636,440834,466522,492710,519407,546623,574368,602652,631486,660881,690847,721396,752539,784287,816652,849646,883282,917572,952528,988164,1024493,1061528,1099283,1137772,1177009,1217009,1257787,1299357,1341736,1384938,1428980,1473878,1519649,1566310,1613878,1662371,1711807,1762204,1813581,1865956,1919350,1973782,2029272,2085841,2143509,2202298,2262230,2323327,2385612,2449108,2513838,2579827,2647099,2715679,2785592,2856864,2929521,3003591,3079101,3156079,3234553,3314553,3396108,3479249,3564006,3650411,3738496,3828293,3919836,4013158,4108295,4205281,4304153,4404947,4507700,4612451,4719238,4828101,4939081,5052218,5167555,5285134,5404999,5527194,5651764,5778756,5908217,6040195,6174739,6311898,6451723,6594267,6739582,6887722,7038741,7192697,7349645,7509645,7672755,7839037,8008550,8181360,8357528,8537121,8720206,8906850,9097122,9291093,9488834,9690420,9895925,10105424,10318997,10536721,10758678,10984949,11215620,11450774,11690500,11934886,12184024,12438004,12696922,12960873,13229955,13504268,13783914,14068996,14359620,14655894,14957928,15265833,15579723,15899723,16225944,16558507,16897534,17243153,17595490,17954676,18320845,18694133,19074677,19462619,19858102,20261274,20672283,21091282,21518427,21953876,22397789,22850332,23311673,23781982,24261434,24750206,25248481,25756441,26274277,26802179,27340343,27888970,28448261,30728962,33022875,35330076,37650643,39984653,42332183,44693313,47068120,49456685,51859086,54275404,56705720,59150114,61608662,64081444,66568543,69070043,71586026,74116577,76661780,79221720,81796481,84386151,86990814,89610559,92245472,94895641,97561155,100242102,102938572,105650654,108378439,111122018,113881482,116656924,119448435,122256110,125080040,127920322,130777048,133650315,136540219,139446854,142370319,145310711,148268128,151242669,154234432,157243517,160270025,163314056,166375712,169455096,172552308,175667454,178800637,181951960,185121530,188309451,191515831,194740776,197984393,201246790,204528077,207828362,211147756,214486370,217844314,221221700,224618642,228035252,231471644,234927932,238404233,241900661,245417334,248954368,252511882,256089994,259688822,263308489,266949113,270610816,274293720,277997949,281723625,285470873,289239817,293030583,296843298,300678089,304535083,308414409,312316196,316240575,320187676,324157630,328150571,332166630,336205943,340268643,344354867,348464749,352598428,356756041,360937726,365143623,369373873,373628615,377907993,382212148,386541224,390895366,395274718,399679426,404109638,408565501,413047163,417554774,422088484,426648444,431234807,435847724,440487350,445153840,449847348,454568032,459316049,464091557,468894715,473725683,478584623,483471696,488387065,493330894,498303347,503304592,508334793,513394120,518482739,523600822,528748539,533926061,539133561,544371212,549639189,554937668,560266825,565626838,571017885,576440146,581893803,587379036,592896028,598444964,604026028,609639407,615285287,620963857,626675305,632419823,638197602,644008834,649853713,655732434,661645192,667592186,673573612,679589671,685640564,691726490,697847655,704004260,710196513,716424619,722688785,728989221,735326136,741699743,748110252,754557878,761042836,767565342,774125613,780723869,787360328,794035212,800748744,807501147,814292646,821123468,827993841,834903993,841854155,848844559,855875437,862947023,870059555,877213267,884408400,891645192,898923886,906244723,913607948,921013806,928462544,935954410,943489654,951068527,958691281,966358172,974069453,981825383,989626220,997472224,1005363656,1013300779,1021283859,1029313160,1037388951,1045511501,1053681081,1061897962,1070162419,1078474727,1086835164,1095244007,1103701538,1112208038,1120763790,1129369081,1138024195,1146729423,1155485055,1164291381,1173148696,1182057295,1191017474,1200029533,1209093772,1218210493,1227380000,1236602598,1245878595,1255208300,1264592024,1274030079,1283522781,1293070446,1302673392,1312331939,1322046409,1331817125,1341644414,1351528603,1361470022,1371469001,1381525874,1391640977,1401814646,1412047221,1422339042,1432690453,1443101799,1453573426,1464105684,1474698924,1485353498,1496069763,1506848075,1517688793,1528592279,1539558896,1550589010,1561682988,1572841201,1584064019,1595351818,1606704973,1618123862,1629608867,1641160371,1652778757,1664464414,1676217731,1688039099,1699928914,1711887570,1723915467,1736013005,1748180588,1760418622,1772727514,1785107674,1797559515,1810083452,1822679904,1835349288,1848092028,1860908549};
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

    //static unsigned char pattern[] = {0x00, 0x00, 0x00, 0x00, 0x20, 0xF0, 0x29, 0x06, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x08, 0x00, 0x1A, 0x00, 0x75, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFF, 0x1D, 0x00, 0x00, 0x00};
    //static unsigned char pattern2[] = {0x00, 0x00, 0x00, 0x00, 0x20, 0xF0, 0x29, 0x06, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x08, 0x00, 0x1A, 0x00, 0x75, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFF, 0x1D, 0x00, 0x00, 0x00};
    static unsigned char pattern[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFF, 0x1D, 0x00, 0x00, 0x00};
    static bool check_pattern( unsigned char* input ){
        bool p1 = true;
        bool p2 = false;
        for( int i=0; i< sizeof(pattern); i++){
            p1 &= input[i] == pattern[i];//  || pattern[i]  == 0x0F;
            //p2 &= input[i] == pattern2[i];// || pattern2[i] == 0x0F;
            if( !(p1 || p2) ) return false;
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
                    if( input[i] == 0xFB ){
                        int idx_pat = (idx+i-6);
                        mem_file.seekg( idx_pat );
                        unsigned char pat[ sizeof(pattern) ];
                        mem_file.read( (char*) pat, sizeof(pattern) );
                        if( check_pattern( pat ) ){
                            exp_memory_offset = idx_pat + sizeof(pattern);
                            std::cout << "found at " << std::hex << exp_memory_offset << std::endl;
                            return true;
                        }
                    }
                    /*if( input[i] == 0x75 ){
                        int idx_pat = (idx+i-18);
                        //std::cout << "potential match: 0x" << std::hex << idx_pat << std::endl;
                        mem_file.seekg( idx_pat );
                        unsigned char pat[ sizeof(pattern) ];
                        mem_file.read( (char*) pat, sizeof(pattern) );
                        if( check_pattern( pat ) ){
                            exp_memory_offset = idx_pat + sizeof(pattern);
                            std::cout << "found at " << std::hex << exp_memory_offset << std::endl;
                            return true;
                        }
                    }*/
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

        int last_exp = 0;
        int last_action = 0;
        int current_level = 0;
        int target_level = 500;

        int data_points = 0;
        double exp_history[1440]; // 1 per minute = 24hrs
        double exp_hour_history[1440]; // 1 per minute = 24hrs

        void Update(int current_exp){
            if( reset ){
                start_time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
                start_exp = current_exp;
                last_exp = current_exp;
                last_update = start_time;
                current_level = 0;
                int i = 0;
                while( exp_table[i] < current_exp ){
                    current_level += 1;
                    i += 1;
                }
                target_level = current_level + 1;
                
                double exp_percent = get_exp_percent(current_exp);

                for(int i = 0; i < 1440; i++){
                     exp_history[i] = exp_percent;
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

            if( last_exp != current_exp ) { //gained exp
                last_action = current_exp - last_exp;
                if( current_exp > exp_table[current_level] ){
                    current_level += 1;
                }
            }
            last_exp = current_exp;

            if( current_time - last_update > 60){
                last_update += 60;
                for(int i = 0; i < 1439; i++){
                    exp_history[i] = exp_history[i+1];
                    exp_hour_history[i] = exp_hour_history[i+1];
                }
                double exp_percent = get_exp_percent(current_exp);
                exp_history[1439] = exp_percent;
                exp_hour_history[1439] = xp_hour;
                data_points += 1;
                if (data_points >= 1440 ) data_points = 1440;
            }
        }

        double get_exp_percent(int current_exp){
            return (double)(current_exp - exp_table[current_level-1]) / (double)(exp_table[current_level] - exp_table[current_level-1]);
        }

        void Draw(int current_exp, const char* title ){
            ImGui::PushID( title );
            ImGui::SeparatorText( title );
            double exp_percent = get_exp_percent(current_exp);
            ImGui::Text( "Lv %d (%.1f%%)",  current_level, exp_percent*100);
            
            ImGui::Text( "Gained %d",  current_exp - start_exp );
            ImGui::SameLine();
            ImGui::Text( "XP/h %.2f",  xp_hour );
            ImGui::InputInt("target level", &target_level);
            if( target_level <= 1 ) target_level = 1;
            if( target_level >= 500 ) target_level = 500;

            if( xp_hour > 0 ) {
                int target_exp = exp_table[target_level-1];
                int required_exp = target_exp-current_exp;
                ImGui::Text( "TTL %.2fh",  required_exp/xp_hour );
                ImGui::SameLine();
                ImGui::Text( "ATL %.2f",  required_exp/(double)last_action );
                ImGui::SameLine();
                ImGui::Text( "ETL % dxp", required_exp);

                static ImPlotAxisFlags xflags = (ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight) & ~ImPlotAxisFlags_NoTickMarks & ~ImPlotAxisFlags_NoGridLines;
                static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight;
                static ImPlotAxisFlags yflags2 = ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight;
                static double tick_gap[] = { 60,120,180,240,300,360,420,480,540,600,660,720,780,840,900,960,1020,1080,1140,1200,1260,1320,1380 };
                if( ImPlot::BeginPlot("XP/h", ImVec2(-1,64), ImPlotFlags_::ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame) ){

                    //ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
                    ImPlot::SetupAxes("Minutes","XP/h",xflags,yflags);
                    ImPlot::SetupAxisTicks(ImAxis_X1, tick_gap, 22 ,nullptr, false); 
                    ImPlot::SetupAxis(ImAxis_Y2, "Y-Axis 2", yflags2 );
                    ImPlot::SetupAxisLimits(ImAxis_Y2, 0, 1);

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);             
                    ImPlot::PlotLine("Xp/H", &exp_hour_history[1440-data_points], data_points);

                   
                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                    ImPlot::PlotLine("XP%", &exp_history[1440-data_points], data_points);

                    ImPlot::EndPlot();
                }
            }
            
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