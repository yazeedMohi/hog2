/*
 *  $Id: Driver.cpp
 *  hog2
 *
 *  Created by Nathan Sturtevant on 5/31/05.
 *  Modified by Nathan Sturtevant on 11/13/21.
 *
 * This file is part of HOG2. See https://github.com/nathansttt/hog2 for licensing information.
 *
 */

#include <cstring>
#include "Common.h"
#include "Driver.h"
#include "Timer.h"
#include "SVGUtil.h"
#include "Hexagon.h"
#include "Combinations.h"
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
//#include "matplotlibcpp.h"
//namespace plt = matplotlibcpp;

using namespace std;

int cutoff = INT_MAX, maxPuzzlesPerCat = 3000, addedPieces = 2;//673;//INT_MAX
bool recording = false, svg = true, dotted = true, doColorSets = true, practiceMode = false;//true
bool saveAndExit = false;
string filename, outputFile;
Hexagon h;
HexagonState hs;
vector<HexagonSearchState> goals;

vector<vector<HexagonAction>> acts;
HexagonSearchState hss, init;
HexagonEnvironment he;
int currDepth = 0;
long expansionsGlob = 0;//, generetions = 0, leaves = 0;

int main(int argc, char* argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	InstallHandlers();
	RunHOGGUI(argc, argv, 512, 1024);
	return 0;
}

/**
 * Allows you to install any keyboard handlers needed for program interaction.
 */
void InstallHandlers()
{
	//	InstallKeyboardHandler(MyDisplayHandler, "Record", "Record a movie", kAnyModifier, 'r');
	InstallKeyboardHandler(MyDisplayHandler, "Save goals", "Save all the currently generated goals", kAnyModifier, 's');
	InstallKeyboardHandler(MyDisplayHandler, "Next Action", "Take Next action", kAnyModifier, ']');
	InstallKeyboardHandler(MyDisplayHandler, "Prev Action", "Take prev action", kAnyModifier, '[');
	InstallKeyboardHandler(MyDisplayHandler, "Next", "Next DFS step", kAnyModifier, 'n');
	InstallKeyboardHandler(MyDisplayHandler, "All", "Get All Goals", kAnyModifier, 'a');
	InstallKeyboardHandler(MyDisplayHandler, "Flip", "Flip Board", kAnyModifier, 'f');
	InstallKeyboardHandler(MyDisplayHandler, "Rotate", "Rotate Board", kAnyModifier, 'r');
    InstallKeyboardHandler(MyDisplayHandler, "Analyze1", "Analyze which piece to remove", kAnyModifier, 'p');
    InstallKeyboardHandler(MyDisplayHandler, "Search", "Constraint space search", kAnyModifier, 'c');
	InstallKeyboardHandler(MyDisplayHandler, "Analyze2", "Analyze which pieces to make unflippable", kAnyModifier, 'o');
	InstallKeyboardHandler(MyDisplayHandler, "Get Coordinates", "Get baseline coordinates of all pieces", kAnyModifier, '=');

	
	InstallCommandLineHandler(MyCLHandler, "-loadPuzzle", "-loadPuzzle <file>", "Load level from file.");
    InstallCommandLineHandler(MyCLHandler, "-loadSolution", "-loadSolution <file>", "Load solution from file.");
    InstallCommandLineHandler(MyCLHandler, "-comparePuzzles", "-comparePuzzles <directory>", "Load all files from directory and compare them to generated goals.");
	InstallCommandLineHandler(MyCLHandler, "-svg", "-svg <file>", "Write SVG to file. Also requires that a file is loaded.");

//	InstallWindowHandler(MyWindowHandler);
//	InstallMouseClickHandler(MyClickHandler);
}

bool MyClickHandler(unsigned long, int, int, point3d p, tButtonType , tMouseEventType e)
{
    return false;
}


void MyWindowHandler(unsigned long windowID, tWindowEventType eType)
{
	if (eType == kWindowDestroyed)
	{
		printf("Window %ld destroyed\n", windowID);
		RemoveFrameHandler(MyFrameHandler, windowID, 0);
	}
	else if (eType == kWindowCreated)
	{
		printf("Window %ld created\n", windowID);
		InstallFrameHandler(MyFrameHandler, windowID, 0);
		SetNumPorts(windowID, 1);
		ReinitViewports(windowID, {-1, -1, 0, 1}, kScaleToSquare);
		AddViewport(windowID, {0, -1, 1, 1}, kScaleToSquare);
		if (true)
		{
//			h.LoadSolution(filename.c_str(), hs);
//			h.LoadPuzzle(filename.c_str(), hs);
		}
		acts.resize(1);
		he.GetActions(hss, acts[0]);
		currDepth = 0;
		
		if (saveAndExit)
		{
			Graphics::Display d;
			h.Draw(d);
			h.Draw(d, hs);
			MakeSVG(d, outputFile.c_str(), 1024, 1024);
			exit(0);
		}
	}
}


void MyFrameHandler(unsigned long windowID, unsigned int viewport, void *)
{
	Graphics::Display &d = GetContext(windowID)->display;
	if (viewport == 0)
	{
		he.Draw(d, hss);
//		h.Draw(d);
//		h.Draw(d, hs);
	}
	if (viewport == 1)
	{
//		h.DrawSetup(d);
	}
}

int forbiddenPiece;

bool AddGoal(HexagonSearchState s)
{
//    HexagonSearchState tmp = s;
//    for (int y = 0; y < 6; y++)
//    {
//
//        Graphics::Display d;
//
//        string fileName = "/Users/yazeedsabil/Desktop/svgs2/" + to_string(y+1) + ".svg";
//
//    //    cout << fileName;
//
//        he.ConvertToHexagonState(tmp, hs);
//        h.Draw(d);
//        h.Draw(d, hs);
//        MakeSVG(d, fileName.c_str(), 1024, 1024);
//        he.RotateCW(tmp);
//    }
    
//    assert(false);
    
    // faster ways to do this, but they require the hash function I haven't written yet
    for (int x = 0; x < goals.size(); x++)
    {
        HexagonSearchState tmp = goals[x];
        for (int y = 0; y < 6; y++)
        {
            if (s == tmp)
            {
//                cout << "DUP " << y << "\n";
                return false;
            }
            he.RotateCW(tmp);
        }
        he.Flip(tmp);
        for (int y = 0; y < 6; y++)
        {
            if (s == tmp)
            {
//                cout << "DUPF" << x << "\n";
                return false;
            }
            he.RotateCW(tmp);
        }
    }
    
    s.index = goals.size();
    s.forbiddenPiece = forbiddenPiece;
    he.BuildAdjacencies(s);
    
//    for (int r = 0; r < 10; r++) {
//        cout << s.state[0].piece << " " << s.state[r].piece << " | " << s.edgeAdjacencies[r] << " " << s.cornerAdjacencies[r] << "\n";
//    }
    
    goals.push_back(s);
    
//    cout << "Generating SVG..";
//    
//    Graphics::Display d;
//    
//    string fileName = "/Users/yazeedsabil/Desktop/svgs_clean/" + to_string(goals.size()) + ".svg";
//    
////    cout << fileName;
//    
//    he.ConvertToHexagonState(s, hs);
//    h.Draw(d);
//    h.Draw(d, hs);
//    MakeSVG(d, fileName.c_str(), 1024, 1024);

    return true;
}

void CheckDuplicateGoals()
{
    cout << "Running duplicate analysis..\n";

    bool dup = false;
    
    for (int x = 0; x < goals.size(); x++)
    {
        HexagonSearchState s = goals[x];

        for (int y = x+1; y < goals.size(); y++)
        {
            if(x == y) continue;
            
            HexagonSearchState tmp = goals[y];
            for (int k = 0; k < 6; k++)
            {
                if (s == tmp)
                {
                    cout << "Duplicate caught:" << x << "," << y << "," << k << "\n";
                    dup = true;
                }
                he.RotateCW(tmp);
            }
            he.Flip(tmp);
            for (int k = 0; k < 6; k++)
            {
                if (s == tmp)
                {
                    cout << "Duplicate caught F:" << x << "," << y << "," << k << "\n";
                    dup = true;
                }
                he.RotateCW(tmp);
            }
        }
    }
    
    if(!dup) cout << "No duplicates found\n";
}

int MyCLHandler(char *argument[], int maxNumArgs)
{
	if (strcmp(argument[0], "-loadSolution") == 0)
	{
		if (maxNumArgs > 1)
		{
			h.LoadSolution(argument[1], hs);
			return 2;
		}
		printf("Error: too few arguments to -loadSolution. Input file required\n");
		exit(0);
	}
    else if (strcmp(argument[0], "-loadPuzzle") == 0)
    {
        if (maxNumArgs > 1)
        {
            h.LoadPuzzle(argument[1], hs);
            return 2;
        }
        printf("Error: too few arguments to -loadPuzzle. Input file required\n");
        exit(0);
    }
    else if (strcmp(argument[0], "-comparePuzzles") == 0)
    {
        if (maxNumArgs > 1)
        {
//            array<array<uint64_t, numPieces>, (14*6*2+1)> locationsNew = he.GetLocationTable();
            vector<HexagonState> fileGoals, notFoundFiles;
            vector<int> matchedGoals;

            array<tFlipType, numPieces> toFlip;
            for (int x = 0; x < numPieces; x++) toFlip[x] = kHoles;
            he.SetFlippable(toFlip);

            he.SetPieces({kHexagon, kElbow, kLine, kMountains, kWrench, kTriangle, kHook, kButterfly, kTrapezoid, kTrapezoid});

            static int totalGoals = 0;
            static int totalExpansions = 0;
            totalGoals = totalExpansions = 0;
            goals.clear();
            hss.Reset();
            acts.resize(1);
            he.GetActions(hss, acts[0]);
            currDepth = 0;
//            Timer t;
//            t.StartTimer();
            while (true)
            {
                if (acts[currDepth].size() > 0)
                {
                    he.ApplyAction(hss, acts[currDepth].back());
                    currDepth++;
                    acts.resize(currDepth+1);
                    he.GetActions(hss, acts.back());
                    totalExpansions++;
                }
                else if (currDepth > 0) {
                    currDepth--;
                    he.UndoAction(hss, acts[currDepth].back());
                    acts[currDepth].pop_back();
                }
                else {
//                    t.EndTimer();
                    printf("& %lu & %d & %1.2f \\\\\n", goals.size(), totalGoals, totalExpansions/1000000.0);
                    //printf("%1.2f elapsed\n", t.GetElapsedTime());
                    break;
                }
                if (he.GoalTest(hss))
                {
                    AddGoal(hss);
//                    goals.push_back(hss);
                    totalGoals++;
                    
                    printf("& %lu & %d & %1.2f \\\\\n", goals.size(), totalGoals, totalExpansions/1000000.0);

                }
            }

//            while (true)
//            {
//                cout << currDepth << "\n";
//                if (acts[currDepth].size() > 0)
//                {
//                    he.ApplyAction(hss, acts[currDepth].back());
//                    currDepth++;
//                    acts.resize(currDepth+1);
//                    he.GetActions(hss, acts.back());
//                }
//                else if (currDepth > 0) {
//                    currDepth--;
//                    he.UndoAction(hss, acts[currDepth].back());
//                    acts[currDepth].pop_back();
//                }
//                else break;
//
//                if (he.GoalTest(hss))
//                {
//                    AddGoal(hss);
//                }
//            }
//
//            namespace fs = __fs::filesystem;
//
//            const fs::path pathToShow{ argument[1] };
//
//            for (const auto& entry : fs::directory_iterator(pathToShow)) {
//                const auto filenameStr = entry.path().filename().string();
//                if (entry.is_regular_file()) {
//                    if(filenameStr.find(".txt") != string::npos)
//                    {
//                        h.LoadSolution(entry.path().string().c_str(), hs);
//                        fileGoals.push_back(hs);
//                    }
//                }
//            }
//
//            int idx = 0, idx2 = 0;
//            bool found;
//
//            for(auto hex : fileGoals)
//            {
//                idx2 = 0;
//                found = false;
//
//                for(auto tmp : goals)
//                {
//                    for (int y = 0; y < 6; y++)
//                    {
//                        he.ConvertToHexagonState(tmp, hs);
//
//                        if (hex == hs)
//                        {
//                            found = true;
//                            break;
//                        }
//
//                        he.RotateCW(tmp);
//                    }
//
//                    if(found)
//                    {
//                        matchedGoals.push_back(idx2);
////                        cout << "N goal: " << idx2 << "\n";
//                        break;
//                    }
////                    else{
////                        cout << "N miss: " << idx2 << "\n";
////                    }
//
//                    he.Flip(tmp);
//
//                    for (int y = 0; y < 6; y++)
//                    {
//                        he.ConvertToHexagonState(tmp, hs);
//
//                        if (hex == hs)
//                        {
//                            found = true;
//                            break;
//                        }
//
//                        he.RotateCW(tmp);
//                    }
//
//                    if(found)
//                    {
//                        matchedGoals.push_back(idx2);
////                        cout << "F goal: " << idx2 << "\n";
//                        break;
//                    }
////                    else{
////                        cout << "N miss: " << idx2 << "\n";
////                    }
//
//                    idx2++;
//                }
//
//                if(!found)
//                    notFoundFiles.push_back(hex);
//
//                idx++;
//            }
//
//
//            printf("Found goals: %lu | File goals: %lu | Not found file goals: %lu\n", goals.size(), fileGoals.size(), notFoundFiles.size());
//
//            cout << "matched " << matchedGoals.size() << "\n";
//
////            for(auto i : matchedGoals)
////                cout << "" << i << "\n";//
////            int j = 0;
//            for(int i = 0; i < goals.size(); i++)
//            {
//                if(count(matchedGoals.begin(), matchedGoals.end(), i)){
////                    cout << "missed " << i << "\n";
//                }else{
//                    cout << "Goal #" << i << "\n\n";
//
//                    he.ConvertToHexagonState(goals[i], hs);
//                    cout << he.PrintHexagonState(hs)<<"\n\n----------------\n\n";
//
//
//                }
//            }
            
            // goals_raw = list_of_all_goals
            // goals = {}
            
            // foreach(gr in goals_raw)
            //      goals.add(HexagonState(gr))
            
            // foreach(file in directory)
            //      hex = HexagonState(file)
            
            //      foreach(goal in goals) =>
            //          foreach(symmetry in goal) =>
            //              if(hex == symmetry)
            //                  found = true
            //                  break

            //      if(!found) not_found_files.add(file)
            
            
            // foreach(goal in goals)
            //      foreach(file in directory)
            //          hex = HexagonState(file)
            //          foreach(symmetry in hex) =>
            //              if(goal == symmetry)
            //                  found = true
            //                  break

            //      if(!found) not_found_goals.add(goal)

//            h.LoadPuzzle(argument[1], hs);
                
            return 2;
        }
        printf("Error: too few arguments to -comparePuzzles. Input file required\n");
        exit(0);
    }
	else if (strcmp(argument[0], "-svg") == 0)
	{
		if (maxNumArgs > 1)
		{
			saveAndExit = true;
			outputFile = argument[1];
			return 2;
		}
		printf("Error: too few arguments to -svg. Output filename required.\n");
		exit(0);
	}
	return 0;
}

uint64_t oddDots = he.BitsFromArray({1,3,5,8,10,12,14,17,19,21,23,25,27,29,31,33,35,37,38,40,42,44,46,47,49,51,53});
//uint64_t oddDots = he.BitsFromArray({1,3,5,7,8,10,12,14,16,17,19,21,23,25,27,29,31,33,35,37,40,42,44,46,49,51,53});


void GetAllSolutions()
{
    array<tFlipType, numPieces> toFlip;
    for (int x = 0; x < numPieces; x++)
        toFlip[x] = kHoles;
    he.SetFlippable(toFlip);
    
    const vector<tPieceName> allPieces =
    {kLine, kMountains, kWrench, kTriangle, kHook, kSnake, kHexagon, kElbow, kButterfly, kTrapezoid, kTrapezoid};
    

    vector<tPieceName> pieces =
    {kLine, kMountains, kWrench, kTriangle, kHook, kHexagon, kElbow, kTrapezoid, kTrapezoid, kTrapezoid, kTrapezoid};
    
    he.SetPieces(pieces);
    MyDisplayHandler(0, kNoModifier, 'a');
    
    return;

    for (int x = 0; x < allPieces.size() - 1; x++)//allPieces.size() - 1
    {
        pieces = allPieces;

        forbiddenPiece = pieces[x];
        
//        printf("%s%s ", pieceNames[allPieces[x]].c_str(), (toFlip[x]==kSide1)?"1":(toFlip[x]==kSide2)?"2":"");
        pieces.erase(pieces.begin()+x);
        he.SetPieces(pieces);
        int oldCount = goals.size();
        MyDisplayHandler(0, kNoModifier, 'a');
        
        printf("%s: %d ", pieceNames[allPieces[x]].c_str(), goals.size() - oldCount);

        if(goals.size() >= cutoff)
            break;
    }

    if(goals.size() < cutoff)
    {
        pieces = allPieces;
        pieces.pop_back();
        pieces.pop_back();
        
        forbiddenPiece = kTrapezoid;
        he.SetPieces(pieces);
        MyDisplayHandler(0, kNoModifier, 'a');
    }
}

void ConstraintSpaceSearch()
{
    
//    return;
    Timer t;
    t.StartTimer();
    GetAllSolutions();
    t.EndTimer();
    cout<< "elapsed: " << t.GetElapsedTime() << " expansions: " << expansionsGlob << "\n";
//    he.ConstraintSpaceSearch(goals);

}
    
void PerformFullAnalysis()
{
    GetAllSolutions();
    
    vector<vector<HexagonSearchState>> selectedSolutions(6);
    
    int categories = doColorSets ? 6 : 1;

    he.FullAnalysis(goals, selectedSolutions, categories);
    
    cout << "\n------------------------------------------------------\n";
    cout << "Generating full solution SVGs...";
    cout << "\n------------------------------------------------------\n";
    
    mkdir("/Users/yazeedsabil/Desktop/Temp/gen_puzzles/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    for (int i = 0; i < categories; i++) {
        mkdir(("/Users/yazeedsabil/Desktop/Temp/gen_puzzles/"+to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
        vector<int> solCounts(goals.size());
        
        for (int j = 0; j < min(selectedSolutions[i].size(), maxPuzzlesPerCat); j++)
        {
            Graphics::Display d;
        
            string fileName = "/Users/yazeedsabil/Desktop/Temp/gen_puzzles/"+ to_string(i) +"/" + to_string(selectedSolutions[i][j].index) + "-" + to_string(solCounts[selectedSolutions[i][j].index])+ ".svg";
            
//            if(i==5) cout << "idx: " <<selectedSolutions[i][j].index << " constraint: " << selectedSolutions[i][j].constraints[0] << "\n";
        
            solCounts[selectedSolutions[i][j].index]++;
                    
            he.ConvertToHexagonState(selectedSolutions[i][j], hs, true);
            
            hs.forbiddenPiece = selectedSolutions[i][j].forbiddenPiece;
            
            h.Draw(d);
            h.Draw(d, hs);
            
            if(svg)
                MakeSVG(d, fileName.c_str(), 1024, 1024);
        }
    }
    
    
    cout << "\n------------------------------------------------------\n";
    cout << "Finished generating solution SVGs";
    cout << "\n------------------------------------------------------\n";
    
    if(!practiceMode)
    {
        cout << "\n------------------------------------------------------\n";
        cout << "Calculating entropies...";
        cout << "\n------------------------------------------------------\n";
        
        for (int i = 0; i < categories; i++)
        {
            cout << "\nAnalyzing group: " << i << " ("<< selectedSolutions[i].size() << " items)\n";
            for (int j = 0; j < min(selectedSolutions[i].size(), maxPuzzlesPerCat); j++)
            {
                vector<tPieceName> pcs;
                for (int r = 0; r < selectedSolutions[i][j].cnt; r++)
                {
                    tPieceName x = static_cast<tPieceName>(selectedSolutions[i][j].state[r].piece);
                    pcs.push_back(x);
                }
                
                he.SetPieces(pcs);
                
                init = he.GetInitState(selectedSolutions[i][j]);
                selectedSolutions[i][j].entropy = selectedSolutions[i][j].entropyWithAddedPieces = he.GetEntropy(init);
                //            cout << "Entropy [" << selectedSolutions[i][j].index << "] = " << hss.entropy << "\n";
            }
        }
        
        cout << "\n------------------------------------------------------\n";
        cout << "Finished calculating entropies";
        cout << "\n------------------------------------------------------\n";
    }
    
    cout << "\n------------------------------------------------------\n";
    cout << "Generating inital state SVGs...";
    cout << "\n------------------------------------------------------\n";
    
    mkdir("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_init/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    for (int i = 0; i < categories; i++) {
        mkdir(("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_init/" + to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        vector<int> solCounts(goals.size());
        for (int j = 0; j < min(selectedSolutions[i].size(), maxPuzzlesPerCat); j++) {
            Graphics::Display d;
        
            string fileName = "/Users/yazeedsabil/Desktop/Temp/gen_puzzles_init/" + to_string(i) + "/" + to_string(selectedSolutions[i][j].entropy) + "-" + to_string(selectedSolutions[i][j].index) + "-" + to_string(solCounts[selectedSolutions[i][j].index]) + ".svg";
        
            solCounts[selectedSolutions[i][j].index]++;
            
//            selectedSolutions[i][j].dots = i == 0 ? 0 : oddDots;
                    
            he.ConvertToHexagonState(selectedSolutions[i][j], hs);
            
            hs.forbiddenPiece = selectedSolutions[i][j].forbiddenPiece;
            hs.entropy = selectedSolutions[i][j].entropy;
            
            h.Draw(d);
            h.Draw(d, hs);
            
            if(svg)
                MakeSVG(d, fileName.c_str(), 1024, 1024);
        }
    }
    
    cout << "\n------------------------------------------------------\n";
    cout << "Finished generating inital state SVGs";
    cout << "\n------------------------------------------------------\n";
    
    if(practiceMode)
    {
        cout << "\n------------------------------------------------------\n";
        cout << "Generating easier init state SVGs...";
        cout << "\n------------------------------------------------------\n";
        
        mkdir("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_init_modified/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
        for (int i = 0; i < categories; i++) {
            mkdir(("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_init_modified/" + to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            vector<int> solCounts(goals.size());
            int easier = 0;
            //        for(int easier = 0; easier < 2; easier++)
            for (int j = 0; j < min(selectedSolutions[i].size(), maxPuzzlesPerCat); j++) {
                Graphics::Display d;
                
                vector<tPieceName> pcs;
                for (int r = 0; r < selectedSolutions[i][j].cnt; r++)
                {
                    tPieceName x = static_cast<tPieceName>(selectedSolutions[i][j].state[r].piece);
                    pcs.push_back(x);
                }
                
                he.SetPieces(pcs);
                
                
                solCounts[selectedSolutions[i][j].index]++;
                
                //            selectedSolutions[i][j].dots = i == 0 ? 0 : oddDots;
                
                he.AddPiecesToInitState(selectedSolutions[i][j], addedPieces, easier == 1);
                
                he.ConvertToHexagonState(selectedSolutions[i][j], hs);
                
                init = he.GetInitState(selectedSolutions[i][j], &selectedSolutions[i][j].addedInitPieces);
                selectedSolutions[i][j].entropyWithAddedPieces = he.GetEntropy(init, 0);
                
                hs.forbiddenPiece = selectedSolutions[i][j].forbiddenPiece;
                hs.entropy = selectedSolutions[i][j].entropyWithAddedPieces;
                
                h.Draw(d);
                h.Draw(d, hs);
                
                //            hss.addedInitPieces.clear();
                
                string fileName = "/Users/yazeedsabil/Desktop/Temp/gen_puzzles_init_modified/" + to_string(i) + "/" + to_string(selectedSolutions[i][j].entropyWithAddedPieces) + "-" + to_string(selectedSolutions[i][j].index) + "-" + to_string(solCounts[selectedSolutions[i][j].index]) + (easier == 0 ? " - HARDER - " : " - EASIER - ") + ".svg";
                
                
                if(svg)
                    MakeSVG(d, fileName.c_str(), 1024, 1024);
            }
        }
        
        cout << "\n------------------------------------------------------\n";
        cout << "Finished generating modified inital state SVGs";
        cout << "\n------------------------------------------------------\n";
        
        cout << "\n------------------------------------------------------\n";
        cout << "Generating entropy inital state SVGs...";
        cout << "\n------------------------------------------------------\n";
        
        //    mkdir("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_entropy/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
        //    short mode = 0b1;
        for(short mode : {0b1, 0b10, 0b100})// = 1; mode < 7; mode++)
        {
            mkdir(("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_entropy_" + to_string(mode)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            
            for (int i = 0; i < categories; i++)
            {
                mkdir(("/Users/yazeedsabil/Desktop/Temp/gen_puzzles_entropy_" + to_string(mode) + "/" + to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                vector<int> solCounts(goals.size());
                for (int j = 0; j < min(selectedSolutions[i].size(), maxPuzzlesPerCat); j++) {
                    Graphics::Display d;
                    
                    vector<tPieceName> pcs;
                    for (int r = 0; r < selectedSolutions[i][j].cnt; r++)
                    {
                        tPieceName x = static_cast<tPieceName>(selectedSolutions[i][j].state[r].piece);
                        pcs.push_back(x);
                    }
                    
                    he.SetPieces(pcs);
                    
                    init = he.GetInitState(selectedSolutions[i][j], &selectedSolutions[i][j].addedInitPieces);
                    
                    float newEntropy = he.GetEntropy(init, mode);
                    float entropyDiff = selectedSolutions[i][j].entropyWithAddedPieces - newEntropy;
                    
                    //                if(entropyDiff < 0)
                    //                {
                    //                    cout << "DIFF: " << he.GetEntropy(init, 0) << " => " << he.GetEntropy(init, mode) << "\n";
                    //                }
                    
                    string fileName = "/Users/yazeedsabil/Desktop/Temp/gen_puzzles_entropy_" + to_string(mode) + "/" + to_string(i) + "/" + to_string(entropyDiff) + "-" + to_string(selectedSolutions[i][j].index) + "-" + to_string(solCounts[selectedSolutions[i][j].index]) + ".svg";
                    
                    solCounts[selectedSolutions[i][j].index]++;
                    
                    //            selectedSolutions[i][j].dots = i == 0 ? 0 : oddDots;
                    
                    he.ConvertToHexagonState(selectedSolutions[i][j], hs);
                    
                    hs.forbiddenPiece = selectedSolutions[i][j].forbiddenPiece;
                    hs.entropy = newEntropy;
                    
                    h.Draw(d);
                    h.Draw(d, hs);
                    
                    if(svg)
                        MakeSVG(d, fileName.c_str(), 1024, 1024);
                }
            }
        }
        cout << "\n------------------------------------------------------\n";
        cout << "Finished entropy inital state SVGs";
        cout << "\n------------------------------------------------------\n";
    }
    
    cout << "\n------------------------------------------------------\n";
    cout << "END OF PROGRAM";
    cout << "\n------------------------------------------------------\n";
    
    return;
//
//    cout<<"HERE " << selectedSolutions[5].size() << "\n";
//    vector<int> solCounts(goals.size());
//
//    for (int j = 0; j < selectedSolutions[5].size(); j++)
//    {
//        HexagonSearchState g = selectedSolutions[5][j];
//        cout<< g.index << " " << g.allConstraints.size() << "\n";
//        for(int c1 = 0; c1 < g.allConstraints.size() - 1; c1++)
//        {
//            for(int c2 = c1+1; c2 < g.allConstraints.size(); c2++)
//            {
//                if(((g.allConstraints[c1] / 10) % numPieces) == ((g.allConstraints[c2] / 10) % numPieces) && ((g.allConstraints[c1] / 10) / numPieces) == ((g.allConstraints[c2] / 10) / numPieces))//TODOX if you wanna do cross color constraints then make sure to account for that here
//                    continue;
//
//                HexagonSearchState newGoal;
//                newGoal.state = g.state;
//                newGoal.cnt = g.cnt;
//                newGoal.bits = g.bits;
//                newGoal.index = g.index;
//
//                newGoal.constraints.push_back(g.allConstraints[c1]);
//                newGoal.constraints.push_back(g.allConstraints[c2]);
//
//                Graphics::Display d;
//
//                string fileName = "/Users/yazeedsabil/Desktop/gen_puzzles/5/" + to_string(newGoal.index) + "-" + to_string(solCounts[newGoal.index])+ ".svg";
//
//                solCounts[newGoal.index]++;
//
//                he.ConvertToHexagonState(newGoal, hs);
//                h.Draw(d);
//                h.Draw(d, hs);
//                if(svg)
//                   MakeSVG(d, fileName.c_str(), 1024, 1024);
//            }
//        }
//    }
//
//    solCounts = vector<int>(goals.size());
//
//    for (int j = 0; j < selectedSolutions[5].size(); j++)
//    {
//        HexagonSearchState g = selectedSolutions[5][j];
//        cout<< g.index << " " << g.allConstraints.size() << "\n";
//        for(int c1 = 0; c1 < g.allConstraints.size() - 1; c1++)
//        {
//            for(int c2 = c1+1; c2 < g.allConstraints.size(); c2++)
//            {
//                if(((g.allConstraints[c1] / 10) % numPieces) == ((g.allConstraints[c2] / 10) % numPieces) && ((g.allConstraints[c1] / 10) / numPieces) == ((g.allConstraints[c2] / 10) / numPieces))//TODOX if you wanna do cross color constraints then make sure to account for that here
//                    continue;
//
//                HexagonSearchState newGoal;
//                newGoal.state = g.state;
//                newGoal.cnt = g.cnt;
//                newGoal.bits = g.bits;
//                newGoal.index = g.index;
//
//                newGoal.constraints.push_back(g.allConstraints[c1]);
//                newGoal.constraints.push_back(g.allConstraints[c2]);
//
//                Graphics::Display d;
//
//                string fileName = "/Users/yazeedsabil/Desktop/gen_puzzles_init/5/" + to_string(newGoal.index) + "-" + to_string(solCounts[newGoal.index])+ ".svg";
//
//                solCounts[newGoal.index]++;
//
//                he.ConvertToHexagonState(newGoal, hs);
//                h.Draw(d);
//                h.Draw(d, hs);
//                if(svg)
//                    MakeSVG(d, fileName.c_str(), 1024, 1024);
//            }
//        }
//    }
}

void AnalyzeWhichPiecesToFlip()
{
	Combinations<6> c;
	int items[3];
	for (uint64_t i = 0; i < c.MaxRank(3)*8; i++)
	{
		uint8_t order = i%8;
		// Get which itesms to be able to flip this time
		c.Unrank(i/8, items, 3);

		// Clear and then set the items
		array<tFlipType, numPieces> toFlip;
		for (int x = 0; x < numPieces; x++)
			toFlip[x] = kHoles;
		for (int x = 0; x < 3; x++)
		{
			switch(order&1)
			{
				case 0:
					toFlip[items[x]] = kSide1;
					break;
				case 1:
					toFlip[items[x]] = kSide2;
					break;
			}
			order/=2;
		}
		he.SetFlippable(toFlip);

		const vector<tPieceName> allPieces =
		{kLine, kMountains, kWrench, kTriangle, kHook, kSnake, kHexagon, kElbow, kButterfly, kTrapezoid, kTrapezoid};
		vector<tPieceName> pieces;

		printf("\n---- %s%s, %s%s, %s%s ----\n",
			   pieceNames[allPieces[items[0]]].c_str(), (toFlip[items[0]]==kSide1)?"1":(toFlip[items[0]]==kSide2)?"2":"",
			   pieceNames[allPieces[items[1]]].c_str(), (toFlip[items[1]]==kSide1)?"1":(toFlip[items[1]]==kSide2)?"2":"",
			   pieceNames[allPieces[items[2]]].c_str(), (toFlip[items[2]]==kSide1)?"1":(toFlip[items[2]]==kSide2)?"2":"");

		for (int x = 0; x < 1; x++)
		{
			pieces = allPieces;

			printf("%s%s ", pieceNames[allPieces[x]].c_str(), (toFlip[x]==kSide1)?"1":(toFlip[x]==kSide2)?"2":"");
			pieces.erase(pieces.begin()+x);
			he.SetPieces(pieces);
			MyDisplayHandler(0, kNoModifier, 'a');
		}
		// now remove trapezoids
//		pieces = allPieces;
//		pieces.pop_back();
//		pieces.pop_back();
//		printf("%s ", pieceNames[allPieces[9]].c_str());
//		he.SetPieces(pieces);
//		MyDisplayHandler(0, kNoModifier, 'a');
	}
}

int wasEqualSize = 0;

void MyDisplayHandler(unsigned long windowID, tKeyboardModifier mod, char key)
{
	switch (key)
	{
		case 's':
		{
			for (auto g : goals)
			{
				Graphics::Display d;
				d.StartFrame();
				he.Draw(d, g);
				d.EndFrame();
				
			}
			break;
		}
		case 'f':
			he.Flip(hss);
			break;
		case 'r':
			he.RotateCW(hss);
			break;
		case 'n':
		{
			static int totalGoals = 0;
            do
            {
				if (acts[currDepth].size() > 0)
				{
					he.ApplyAction(hss, acts[currDepth].back());
					currDepth++;
					acts.resize(currDepth+1);
					he.GetActions(hss, acts.back());
				}
				else if (currDepth > 0) {
					currDepth--;
					he.UndoAction(hss, acts[currDepth].back());
					acts[currDepth].pop_back();
				}
				else {
					printf("Done!\n");
					break;
				}
				if (he.GoalTest(hss))
				{
					if (AddGoal(hss))
					{
                        he.ConvertToHexagonState(hss, hs);
						totalGoals++;
//						printf("%d total goals\n", totalGoals);
						break;
					}
				}
            } while (true);
		}
            
            // 
			break;
		case 'o':
			AnalyzeWhichPiecesToFlip();
			break;
        case 'p':
            PerformFullAnalysis();
            break;
        case 'c':
            ConstraintSpaceSearch();
            break;
		case '=':
		{
            cout<<"\n\nGENERATING PIECE COORDS..\n\n";
			for (int x = 0; x < numPieces; x++)
			{
				he.GeneratePieceCoordinates((tPieceName)x);
			}
            cout<<"\n\nDONE GENERATING PIECE COORDS\n\n";
            cout<<"\n\GENERATING BOARD COORDS...\n\n";
			he.GenerateBoardBorder();
            cout<<"\n\DONE GENERATING BOARD COORDS\n\n";

			break;
		}
//		{
//			const vector<tPieceName> allPieces =
//			{kHexagon, kElbow, kLine, kMountains, kWrench, kTriangle, kHook, kSnake, kButterfly, kTrapezoid, kTrapezoid};
//			vector<tPieceName> pieces;
//			for (int x = 0; x < 9; x++)
//			{
//				pieces = allPieces;
//				printf("%s ", pieceNames[allPieces[x]].c_str());
//				pieces.erase(pieces.begin()+x);
////				printf("Piece set: ");
////				for (auto i : pieces)
////					printf("%d ", (int)i);
////				printf("\n");
//				he.SetPieces(pieces);
//				MyDisplayHandler(windowID, mod, 'a');
//			}
//			// now remove trapezoids
//			pieces = allPieces;
//			pieces.pop_back();
//			pieces.pop_back();
//			printf("%s ", pieceNames[allPieces[9]].c_str());
////			printf("Piece set: ");
////			for (auto i : pieces)
////				printf("%d ", (int)i);
////			printf("\n");
//			he.SetPieces(pieces);
//			MyDisplayHandler(windowID, mod, 'a');
//		}
			break;
		case 'a':
		{
			static int totalGoals = 0;
			static int totalExpansions = 0;
			totalGoals = totalExpansions = 0;
//			goals.clear();
			hss.Reset();
            //1,3,5,7,8,10,12,14,16,17,19,21,23,25,27,29,31,33,35,37,40,42,44,46,49,51,53
            //0,2,4,6,7,9,11,13,15,16,18,20,22,24,26,28,30,32,34,36,39,41,43,45,48,50,52
            hss.dots = dotted ? oddDots : 0;
//            cout<< "Dots\n"<<bitset<54>(hss.dots)<<"\n\n";
			acts.resize(1);
			he.GetActions(hss, acts[0]);
			currDepth = 0;
            wasEqualSize = 0;
            
//            cout << "Generating all solutions.." << acts[0].size();
                        
			while (true)
			{
                if (acts[currDepth].size() > 0 && goals.size() < cutoff)
				{
					he.ApplyAction(hss, acts[currDepth].back());
					currDepth++;
					acts.resize(currDepth+1);
					he.GetActions(hss, acts.back());
					totalExpansions++;
                    expansionsGlob++;
//                    cout << currDepth << "\n";
//                    if(currDepth == 10)
//                    {
//                        cout << "\n\n" << he.PrintBits(hss.bits) << "\n\n" << acts.back().size() << "\n";
//                    }
				}
				else if (currDepth > 0 && goals.size() < cutoff)
                {
					currDepth--;
					he.UndoAction(hss, acts[currDepth].back());
					acts[currDepth].pop_back();
				}
				else {
//					t.EndTimer();
//                    CheckDuplicateGoals();

//					printf("& %lu & %d & %1.2f \\\\\n", goals.size(), totalGoals, totalExpansions/1000000.0);
					//printf("%1.2f elapsed\n", t.GetElapsedTime());
					break;
				}
				if (he.GoalTest(hss))
				{
                    cout << "okay goal found\n";
                    int sizeBefore = goals.size();
                    AddGoal(hss);
                    if(goals.size() == sizeBefore)
                    {
                        if(wasEqualSize > 2)
                            break;
                        else
                            wasEqualSize++;
                    }
                    else
                        wasEqualSize = 0;
                        
                    cout << goals.size() << " total goals\n";
                    
					totalGoals++;
				}
			}
            break;
		}
		case ']':
			if (acts[currDepth].size() > 0)
			{
				he.ApplyAction(hss, acts[currDepth].back());
				currDepth++;
				acts.resize(currDepth+1);
				he.GetActions(hss, acts.back());
			}
			else {
				//printf("No more actions to apply at depth %d\n", currDepth);
			}
//			he.ApplyAction(hss, acts[currAct]);
//			currAct = (currAct+1)%acts.size();
//			he.ApplyAction(hss, acts[currAct]);
//			printf("%d\n", currDepth);
			break;
		case '[':
			if (acts[currDepth].size() == 0)
			{
				printf("Nothing to undo\n");
				break;
			}
				
			currDepth--;
			he.UndoAction(hss, acts[currDepth].back());
			acts[currDepth].pop_back();
//
//			he.ApplyAction(hss, acts[currAct]);
//			currAct = (currAct+acts.size()-1)%acts.size();
//			he.ApplyAction(hss, acts[currAct]);
			printf("%d\n", currDepth);
			break;
		default:
			break;
	}
    
    /*
    main
        chooseWhichPiecesToUse
            generateAllPuzzlesForPieceSet
                loopThroughAllPossibleHolePatternsForAllPiecesInTheSet
                    filterGeneratedPuzzlesBasedOnSelectedHolePatternSet
                        storeNumberOfRemainingPuzzlesAfterFiltration
                        
    
     questions:
     - shape of array stored
     - pattern representation
     - which patterns to cover (some patterns are meaningless - dont cover them)
     -
     
    */
    
}




//                for (unsigned int x = 1; x <= locations[piece][0]; x++)
//                {
//                    if((nodeID.bits&locations[piece][x]) != 0) continue;
//                    for (unsigned int y = 1; y <= localized_holes[piece][x][0]; y++)
//                    {
//                        if (((nodeID.dots&~localized_holes[piece][x][y])&locations[piece][x]) == 0)
//                        {
//                            actions.push_back({piece, x});
//                            break;
//                        }
//                    }
//                }

//                for (int t = 0; t < 66; t++)// goes to 66 for hss!
//                {
//                    int fixedT = (t == 0 || t == 1 || t == 9 || t == 10 || t == 11 || t == 21 || t == 65 || t == 64 || t == 56 || t == 55 || t == 54 || t == 44) ? -1 : (t / 11 == 0 ? (t-2) : (t / 11 == 1 ? (t-5) : (t / 11 == 2 || t / 11 == 3 ? (t-6) : (t / 11 == 4 ? (t-7) : (t-10)))));
//
//                    if(fixedT < 0) continue;
//
//                    if ((loc>>fixedT)&1 && (hss.dots>>fixedT)&1)
//                    {
//
//                    }
//                }
