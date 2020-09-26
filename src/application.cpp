#include <application.h>
#include <imgui.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <stb_image.h>

STBIDEF unsigned char *stbi_xload(char const *filename, int *x, int *y, int *frames);

static struct {
    float width = 200.0f;
    float height = 200.0f;
    int mousex = 0;
    int mousey = 0;
    int mouseImagex = 0;
    int mouseImagey = 0;
    int zoom = 100;
    int translatex = 0;
    int translatey = 0;
    bool shiftPressed = false;
    bool ctrlPressed = false;
    glm::vec2 contentPosition;
    glm::vec2 contentSize;
    bool mousePanning = false;

} state;

typedef struct frame {
    int delay = 100;
    GLuint index = 0;
    struct frame *nextFrame = nullptr;
} ImageFrame;

typedef struct {
    int width = 200;
    int height = 200;
    int componentsPerPixel = 3;
    ImageFrame *firstFrame = nullptr;
} LoadedImage;

static std::string vertexGlsl = "#version 150\n\
in vec3 vertex;\
in vec2 texcoord;\
\
uniform mat4 u_projection;\
uniform mat4 u_view;\
\
out vec2 f_texcoord;\
\
void main()\
{\
    gl_Position = u_projection * u_view * vec4(vertex.xyz, 1.0);\
    f_texcoord = texcoord;\
}";

static std::string fragmentGlsl = "#version 150\n\
        uniform sampler2D u_texture;\
\
in vec2 f_texcoord;\
\
out vec4 color;\
\
void main()\
{\
    color = texture(u_texture, f_texcoord);\
}";

static float g_vertex_buffer_data[] = {
    0.5f,  0.5f,  0.0f,  1.0f, 1.0f,  0.0f,
    0.5f, -0.5f,  0.0f,  1.0f, 0.0f,  0.0f,
    -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,  0.0f,
    -0.5f, -0.5f,  0.0f,  0.0f, 0.0f,  0.0f,
};

// File: 'icons.ttf' (5060 bytes)
// Exported using binary_to_compressed_c.cpp
static const char icons_compressed_data_base85[4425+1] =
    "7])#######P-P$c'/###W),##2(V$#Q6>##.FxF>6pY(/Q5)=-'OE/1I[n42R4aM:^,>>#-rEn/VNV=BLUJso)*m<-(NE/1E1XGHu='6v.c68%D(-p/,Jk7D=GIJQ6rEn/P3NP&%a(*H"
    "MY`=-:(m<-)p/K1#-0%J[r)`jN?uu#Fr.bI'-d<Bbi^mAXG:;$,uX&#TT$=(7iA&$+)m<-[v2:/<_[FHNo^sLRX;;$U#`5/o1JuBG>;mLJZ;;$r@^01sH[^IEr5c&nH:;$vrEn/+>00F"
    "jMrZG)B^0137YY#v%S+H@W$=lW#iH-PtgS/VcFVC;;cp0>pT/)g(m<-_uF%4^D-(#RApl8`Yx+#twCEHVle%#JQr.MtwSfL9b@6/b1+7DeA=gL345GMLLH##,x:T.,),##9*BK1'D(v#"
    "36B2#*5YY#6FAqDV#ArM1.RxkW$eY#37c&#kg3SRc:*oC8q9`aKQ)W#82'DE(+NA=>Ujk=0YNh##_j-$.&K21,C?\?$%3PY>Z2c'&BLN`<_Du'&NM-F%>.GR*nuKS.Dt_>$#30Wn$)>>#"
    ":,n+M5_e6/'####Sr[fLNUZ##4,>>#Q$<$#DO1p/0]j-$I)###?d/.$%1$gL*=PcMZYN$5)+elS)PEG)]MZQ#&2G>#5s;g.miBW&7w7>-JVIq.?(Ee+Pg_$'7]<rLT.WlC`ImF*l;(l0"
    "XcaI)[(Ls-P7Yd3o0`.3n`aI)5gaFib5:6&^QMS&EIaQ&@L_Z,A5cZ#O60quV$vs-P(GwLE1NfUK%-?&?qjnL9ZI%bJ'lwKqC#%LL9%IMl`CH-rbCH-(;^;-0(rH-_*rH-SSL).*dCpL"
    "<w:^uZ1sE-WZ<F.Cv)MBcn,B#MZajL;f7/M6X9`MO#Z5rlTrs'alVP&'5L'SO1NbLgamF%`sQ4$Tk`e.+Sl##40s>MLF-###t((vDv]GMa$>Sn:_Y-M<GT;-4[7:.EAR8#D^fq)[kk.L"
    "G'V#$oi>F%:Ru&#;F.%#X:xu-K+FrL%D,s-EqF@Nc4$##qk1eMH;-##=XldMTF6##@=r.NA.gfLxs]/N*4pfLq;50NWXQ##UNb0N,@,gL/.?A%'r`i0r.]R*lE(,)W($v#1:<p%D=K,E"
    "N(n_&R+35&dch_&WRJM'n+i_&5c4R*EVi_&*NC_&Oui_&cN$)*:=j_&8l4R*p*k_&RKGY>ZHk_&:G-5/'AWdF>M_oD2<XG--]V?-=?[L2?G0H-/cOU%-g`=B1F]aH7Y7fGm('B-1%fFH"
    "&8vhF<rY1F#&vLFawZ6D.gcdG;1BI$&;1eG38vLF(*7L2&qqm9$/2eGnm`9CC9N$HRMB@H/`wiC='oF-k&'42@hViFvoUEHM3FGH,C%12G:_(#+mk.#cSq/#TH4.#HB+.#c:F&#XLM=-"
    "Niu8.0E-(#Arj>-)1He-e]u?0^/RgDH2w1B3OHe-rk@VH`&VJDTA+#HkUjV7/4tiCAEMDF@bXMCSpmcEV^<j1d6Ne$rxOcDCW.EF>e3A=w,&REZXh--KD)FIai>R*DSl]>YPf]GXmqfD"
    "7W)?--W^f1/rR.-,#g--Yt;F.=qVqDLAmEI7DB#Hx$tE@c3Uq)vrSfCsAj]Gh7o>-:P?`S3OZQsLK6,Eq;$mB#j+j1:QN>Hm=.^5+v4L#NEV&#$#dT%90OoR^6[>#*AY>#G*EaE;VeV1"
    ",gpKFqC+7DH5TgL'G-##$)>>#K3w>P_mS%#$l^#$dG37MT5W`[vKRV[lQG;9^A@/;;V>'=#K2^=f*Es3qMaZ?\?OJDO.gIm#lB?h%9fSiB''*h+lWg5#q@HP8=#%6#sqdsAD0-Q#+^+pA"
    "j9Hm#IVoE-@/c[[ZTC>]DlGQ8d*RH=q1a;.B'hI?\?C-i<8F4g7ioQV[b+MT..lbD-Lx'<.m7T)0KNI@'F]+Z2.HZV[#9U<.WKXD-1*6EO`ZwG&Y_bpLx;4/MAbl,M[kQ1MW6ErAQ.r`?"
    "IO,Q#$1_A.'$VC?i6oS1#Yp?-=$)N$hpjHQ<E[g<T=e8.?Cn8.$<*+09MglA13P/11>;E/_UOj$lM5/MmB(9MDa0s.*;f2$^h5R*5:IDOVbtE@-4g[$w3;e0(Egj<K]1`?%?voLKP=d="
    "8l%>-skE?NO@tW[bc%K%D-$6#0]DY$/p?['0X5?.mt68.$,Jm#H2/Q#:KXZ/0tjN1'fp?-'^XT&1utN$*:d*.Y*,W7-&m*.n4DNM&cp/1='2$#nSJT0Og+`%f>Gt7,hV8&fpmA#t3.I-"
    "V^oN/tZCHOJLqiL_3tc<MXg5#LFZkL.C&a+u1l&-Y)d2$g='w86ruS&u4]V[,/1lLVRSv7>VFo3VPgKMCqOd&#>GB?Ns9Z-JVoM1jA/t[sVLZ[Z%ZQ1;F0kLM.Y>-n%dm:'$VC?JR#<."
    "'xuD'&HSD=5-S5'WZwM1V14[9:D76Mf),2_cf$qLea;W[JRKE-4@TV[eW5?.=&i01xvmx[)g]^?km[KCG+wW[RIl-=JY<lM?TFY-E,L/)cw/m(fmli'q#nx[5s:Z[#e4Z[hKY][o%Im#"
    "H.c)'-hR5')BvV[(Zt>-@bDe<.A2##&;+N$JbdOOaXL).jG4jLbuG<-gg./MNI%p.n^uN0?[#edxuhQaIvD^[t_=D'VKO,8)Zs20=&(,)<v-Q#Uikr6.7G>#6lGQ1ih_'#rK529:1V&("
    "CrH59Dof88.</bI_J;B-gA&k4Oj%@#tL'58WIc#]mq`W[S'cu-o12Z)gQ=0)A:Mv)07u+M:JD11b+5F%;_/t[)lu8.sM2N$?D`)+4OC,M>ci11f75F%?k/t[-lu8.%sIg%CiwA,8hh,M"
    "D+821b`1k0HDA#-m:&^,oL]>-dk9F%pf8nNE.9v-dk9F%rl8nNG@pV.dk9F%tr8nNIRP8/dk9F%vx8nNKe1p/dk9F%nuq?-rLglAWnT31/<'3(`U>?.q05T8]xn5B=]fX[pUoM11>kW["
    "SG3a0W`jA1,s(m9_Ff4*]&6-5FqkL>_gCs.;7mS1q+b2Mmo.)*05XW[/#cb+TX`=-^ueR&sDb/2)odLMH?UkLXR-29Hwat-B+Y98,F>R*gYcn]#fLP/VWW&5pwic)lU-g2teQ0Y$<ZqA"
    "#'*9Bl@c;3&XV=6)`51Vxg,Q#%UTt1ra0t[aK/r..G)m99Mf88DI6N:MYg?.eMSE-Xm]Qsw$@8.L%kC#%tUhL+i_41E7KU[&aOh#PraW[U-jNCuNu>-7]4uLNLJ+O[=+j1`tk>-X0@k="
    "),kA-2FYC?0Ab,=QoL;-k$wR]i=XA+$b6?.?q;Q8kl>kF`@0<.);+N$5bvm'5?TV[OFHDO5CTV[-e09C=RdP8<QYC?4(J5BE4`T.;se2$+)`@-m2D&0wR5<.t9Xc+JmVE-FVI'Mol3vL"
    "A&3qLOlji0hq_H-A&lC.`+o5B88BM>S)#UMGd&KMOBg;-*]>l$IG6[?egv41`vQ=-o,Dj$>09s$5G:)5i3/MMkP@g%&ENk+KLEm9NVa;.>Y`=-&:VIMXe'KMDrg5#te/@-jH/a+%e-^#"
    "6w,U;V2PW-ka]?T.0=$@s`Qi$IvBX-,)c8T9/:Z-Civ]59ujS80@5?-`^+pA=up18U/e#6lC+j1*YMq14PXD-C2Dt[9$lk0qpkfN5PneO$ELDPIbo(P*?[51tXhWRgde51kVWEOP&QY["
    "mP(HO6TUs.Ifim1V4DwK'6#9B&X2B%BwDQ8[s#?6x&W&5`Bls7c94<$wruDYwHj@0JHs8KAtGm#s-kpLr>T>.c^b>-'(*n%E]72NYX*61vj?t77XfS8>o/Z-8tsr$B@etqR9)tq8%q5U"
    "^2$6#NYWI)F:Gv2qAhD9?*VQ9WCD>#-Gb;339Dm9b_)lLj#0nL'?B49`f-@-#[a5Bnv(H2Qm@g%,GeA:woD#;W=)##eS,<-oS,<-(r(9.O9?Q#&'wf;CHNX(.a3O+_IMmL8NpnL%+p+M"
    "tS#oLH:)=-.2#HM)a5oLGcfr6<>p`=$]bA##K4&>U)OX(#(Jb%/#l]>6:P&#)Lx>-%T,<-%V,<-GIrIM05voL6:)=-;u@Y-b:(F.):Jb%%0Pb%<54O+6c)s@CHNX(>;4O+[72mLHYVpL"
    "%+p+M.``pLH:)=->2#HM;kM91^CLmC2%A6M<nV26[W>2CcuQ>68U'NClS*)*E8[V[h1M9.IuZ55ag:/DeQHX19wx'&x`+I-:$#(&_5R,Em^OfN]U47MS&c?-K5g,M`kbRMb6wlLk^f:1"
    "l/s[0BKl$'cY/&GcuQ>6WDJAG_'C[0E.jT]&%MJ:mLI;8mN.KCK0Ad<cQnc)wQs+;LmLV=/JM>#ha.Q#HdY20Ji[V$oFD>#.VQb%]9fZ#fY((&,M+GMC__58#Jt$MBRGgLPeh58NZ7jM"
    "dw?##>s6B#[eQ][Kiox[+-cv[XqN30gA[0#F*4gNr<;U)88Y^?6PFU]>Fsl&]XMuu@LJ>Pq<^l8#m8AtHJh6W";
    
GLuint LoadShaderProgram(
    const char* vertShaderSrc,
    const char* fragShaderSrc);

#define ICON_MIN_FA 0xe600
#define ICON_MAX_FA 0xe900
#define ICON_FA_ARROW_LEFT u8"\ue800"
#define ICON_FA_ARROW_RIGHT u8"\ue801"

class Application : public AbstractApplication
{
    std::filesystem::path _imageFile;
    std::vector<std::filesystem::path> _paths;
    ImFont *_font;
    ImFont *_icons;
    GLuint _imageShader;
    GLuint _quadBufferIndex;
    
    const int toolbarHeight = 70;
    const int itemWidth = 60;
    
    LoadedImage _image;
    std::mutex _imageMutex;
    
    bool _isLoading = false;

public:
    virtual bool Setup(
        std::filesystem::path const&imagefile)
    {
        if (!imagefile.empty())
        {
            _paths = GetFileInDirectory(imagefile.parent_path());
        
            SwitchImageAsync(imagefile);
        }
        
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();

        ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        if (font != nullptr)
        {
            io.FontDefault = font;
        }
        else
        {
            io.Fonts->AddFontDefault();
        }

        ImFontConfig config;
        config.MergeMode = true;

        static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        io.Fonts->AddFontFromMemoryCompressedBase85TTF(icons_compressed_data_base85, 18.0f, &config, icon_ranges);

        io.Fonts->Build();
        
        io.IniFilename = nullptr;
        io.WantSaveIniSettings = false;
        
        // Setup style
        ImGui::StyleColorsDark();
        auto &style = ImGui::GetStyle();

        style.WindowRounding = 0;
        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(20, 15);
        style.ItemInnerSpacing = ImVec2(40, 40);
        style.ItemSpacing = ImVec2(10, 15);
        style.ChildRounding = 0;
        style.ScrollbarRounding = 0;

        _imageShader = LoadShaderProgram(vertexGlsl.c_str(), fragmentGlsl.c_str());

        glGenBuffers(1, &_quadBufferIndex);
        glBindBuffer(GL_ARRAY_BUFFER, _quadBufferIndex);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        return true;
    }
    
    virtual void Cleanup()
    {
        if (_image.firstFrame->index > 0)
        {
            glDeleteTextures(1, &(_image.firstFrame->index));
            _image.firstFrame->index = 0;
        }
        if (_quadBufferIndex > 0)
        {
            glDeleteBuffers(1, &_quadBufferIndex);
            _quadBufferIndex = 0;
        }
        if (_imageShader > 0)
        {
            glDeleteProgram(_imageShader);
            _imageShader = 0;
        }
    }
    
    virtual void Render3d()
    {
        auto zoom = glm::scale(glm::mat4(1.0f), glm::vec3(state.zoom / 100.0f));
        auto translate = glm::translate(zoom, glm::vec3(state.translatex, state.translatey, 0.0f));
        auto scale = glm::scale(translate, glm::vec3(_image.width, _image.height, 1.0f));
        
        auto projection = glm::ortho(-(state.width/2.0f), (state.width/2.0f), (state.height/2.0f), -(state.height/2.0f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(_imageShader);
        
        if (_image.firstFrame != nullptr)
        {
            glBindTexture(GL_TEXTURE_2D, _image.firstFrame->index);
        }
        glUniformMatrix4fv(glGetUniformLocation(_imageShader, "u_projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(_imageShader, "u_view"), 1, GL_FALSE, glm::value_ptr(scale));

        glBindBuffer(GL_ARRAY_BUFFER, _quadBufferIndex);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        
        if (!_isLoading)
        {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glUseProgram(0);
    }
    
    virtual void Render2d()
    {
        //ImGui::ShowDemoWindow();
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, toolbarHeight));
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("FileInfo", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        
        auto cursor = ImGui::GetCursorPos();
        
        if (!_isLoading)
        {
            const char *filename = _imageFile.string().c_str();
            auto size = ImGui::CalcTextSize(filename, filename + _imageFile.string().size());
            
            ImGui::SetCursorPos(ImVec2(cursor.x + ((ImGui::GetIO().DisplaySize.x - size.x) / 2.0), cursor.y + ImGui::GetStyle().FramePadding.y));
            ImGui::Text("%s", _imageFile.string().c_str());
            
            if (!_paths.empty() && _imageFile != _paths.front())
            {
                ImGui::SetCursorPos(cursor);
                if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(itemWidth, 0)))
                {
                    PreviousImageInDirectory();
                }
            }
            
            if (!_paths.empty() && _imageFile != _paths.back())
            {
                ImGui::SetCursorPos(ImVec2(cursor.x + ImGui::GetContentRegionAvail().x - itemWidth, cursor.y));
                if (ImGui::Button(ICON_FA_ARROW_RIGHT, ImVec2(itemWidth, 0)))
                {
                    NextImageInDirectory();
                }
            }
        }
        else
        {
            const char *filename = _imageFile.string().c_str();
            auto size = ImGui::CalcTextSize("Loading...", "Loading..." + sizeof("Loading..."));
            
            ImGui::SetCursorPos(ImVec2(cursor.x + ((ImGui::GetIO().DisplaySize.x - size.x) / 2.0), cursor.y + ImGui::GetStyle().FramePadding.y));
            ImGui::Text("Loading...");
        }
        ImGui::End();
        
        if (!_isLoading)
        {
            ImGui::SetNextWindowPos(ImVec2(0, toolbarHeight));
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - toolbarHeight));
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Image", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            
            ImGuiMouseCursor current = ImGui::GetMouseCursor();
            ImGui::InvisibleButton("move me", ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - toolbarHeight));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(2);
            }
            else
            {
                ImGui::SetMouseCursor(0);
            }
            ImGuiIO& io = ImGui::GetIO();
            static ImVec2 dragStart, translateStart;
            if (ImGui::IsMouseClicked(0))
            {
                dragStart = ImGui::GetCursorPos();
                translateStart = ImVec2(state.translatex, state.translatey);
            }
            else if (ImGui::IsMouseDragging(0))
            {
                auto diff = ImGui::GetMouseDragDelta(0);
                state.translatex = int(translateStart.x + diff.x);
                state.translatey = int(translateStart.y + diff.y);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }
    }
    
    std::vector<std::filesystem::path> GetFileInDirectory(
        std::filesystem::path const&directory)
    {
        std::vector<std::filesystem::path> paths;
        
        for (auto &p : std::filesystem::directory_iterator(directory))
        {
            if (!IsImage(p))
            {
                continue;
            }
            
            paths.push_back(p);
        }
        
        return paths;
    }
    
    void PreviousImageInDirectory()
    {
        if (_paths.empty())
        {
            return;
        }
        
        auto found = std::find(_paths.rbegin(), _paths.rend(), _imageFile);
        
        if (found == _paths.rend())
        {
            return;
        }
        
        ++found;
        
        if (found == _paths.rend())
        {
            return;
        }
        
        SwitchImageAsync(*found);
    }
    
    void NextImageInDirectory()
    {
        if (_paths.empty())
        {
            return;
        }
        
        auto found = std::find(_paths.begin(), _paths.end(), _imageFile);
        
        if (found == _paths.end())
        {
            return;
        }
        
        ++found;
        
        if (found == _paths.end())
        {
            return;
        }
        
        SwitchImageAsync(*found);
    }
    
    void GotoFirstImageInDirectory()
    {
        SwitchImageAsync(_paths.front());
    }
    
    void GotoLastImageInDirectory()
    {
        SwitchImageAsync(_paths.back());
    }
    
    bool IsImage(
        std::filesystem::path const&file)
    {
        if (file.extension() == ".png") return true;
        if (file.extension() == ".jpg") return true;
        if (file.extension() == ".bmp") return true;
        if (file.extension() == ".tga") return true;
        if (file.extension() == ".gif") return true;
        
        return false;
    }
    
    void SwitchImageAsync(
        std::filesystem::path const&imagefile)
    {
        if (_isLoading)
        {
            return;
        }
        
        std::thread t([&, imagefile]() {
                AbstractApplication::ActivateLoadingContext();
                
                SwitchImage(imagefile);
                
                AbstractApplication::DeactivateLoadingContext();
            });
        t.detach();
    }
    
    bool SwitchImage(
        std::filesystem::path const&imagefile)
    {
        _isLoading = true;
        
        GLuint newImageIndex = 0;
        int w, h, c;
        
        if (imagefile.empty())
        {
            std::cerr << "image filename is not valid" << std::endl;

            _isLoading = false;
            
            return false;
        }
/*
        if (imagefile.extension() == ".gif")
        {
            int frames = 0;
            auto result = stbi_xload(
                imagefile.string().c_str(),
                &w,
                &h,
                &frames);
            
            if (result == nullptr)
            {
                std::cerr << "failed to load gif data from " << imagefile << std::endl;
                
                _isLoading = false;
                
                return false;
            }
        }
*/
        unsigned char *data = stbi_load(
            imagefile.string().c_str(),
            &w,
            &h,
            &c,
            4);
            
        if (data != nullptr)
        {
            glGenTextures(1, &newImageIndex);
            glBindTexture(GL_TEXTURE_2D, newImageIndex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
            glBindTexture(GL_TEXTURE_2D, 0);
            stbi_image_free(data);
            
            GLuint indices[1] = {newImageIndex};
            SetCurrentImage(imagefile, w, h, 4, indices, 1);
            
            _isLoading = false;
            
            return true;
        }
        else
        {
            std::cerr << "failed to load image data from " << imagefile << std::endl;
            
            _isLoading = false;
            
            return false;
        }
    }
    
    void SetCurrentImage(
        std::filesystem::path const&imagefile,
        int width,
        int height,
        int componentsPerPixel,
        GLuint indices[],
        int frameCount)
    {
        std::lock_guard<std::mutex> guard(_imageMutex);
        
        while (_image.firstFrame != nullptr)
        {
            auto tmp = _image.firstFrame;
            _image.firstFrame = _image.firstFrame->nextFrame;
            if (tmp->index > 0)
            {
                glDeleteTextures(1, &(tmp->index));
            }
            delete tmp;
        }
        
        _imageFile = imagefile;
        
        _image.width = width;
        _image.height = height;
        _image.componentsPerPixel = componentsPerPixel;
        
        _image.firstFrame = new ImageFrame();
        _image.firstFrame->index = indices[0];
        
        auto current = _image.firstFrame;
        for (int i = 1; i < frameCount; i++)
        {
            auto tmp = new ImageFrame();
            tmp->index = indices[i];
            current->nextFrame = tmp;
            current = tmp;
        }
    
        float horAspect = 1.0f;
        float verAspect = 1.0f;
        if (_image.width > state.width)
        {
            horAspect = float(state.width) / float(_image.width);
        }
        if (_image.height > (state.height - toolbarHeight - toolbarHeight))
        {
            verAspect = float(state.height - toolbarHeight - toolbarHeight) / float(_image.height);
        }
        
        state.zoom = 100 * std::min(horAspect, verAspect);
        state.translatex = 0;
        state.translatey = 0;
    }
    
    virtual void OnResize(
        int w,
        int h)
    {
        state.width = w;
        state.height = h;
    }
    
    virtual void OnPreviousPressed()
    {
        PreviousImageInDirectory();
    }
    
    virtual void OnNextPressed()
    {
        NextImageInDirectory();
    }
    
    virtual void OnHomePressed()
    {
        GotoFirstImageInDirectory();
    }
    
    virtual void OnEndPressed()
    {
        GotoLastImageInDirectory();
    }
    
    virtual void OnZoom(
        int amount)
    {
        state.zoom += (amount * 10);
        if (state.zoom < 1)
        {
            state.zoom = 1;
        }
    }
};

extern AbstractApplication* CreateApplication();

AbstractApplication* CreateApplication()
{
    return new Application();
}

GLuint LoadShaderProgram(
    const char* vertShaderSrc,
    const char* fragShaderSrc)
{
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    GLint result = GL_FALSE;
    GLint logLength;

    glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
    glCompileShader(vertShader);

    // Check vertex shader
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        std::cout << "compiling vertex shader" << std::endl;
        glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> vertShaderError(static_cast<size_t>((logLength > 1) ? logLength : 1));
        glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
        std::cout << (&vertShaderError[0]) << std::endl;
    }

    glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
    glCompileShader(fragShader);

    // Check fragment shader
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        std::cout << "compiling fragment shader" << std::endl;
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> fragShaderError(static_cast<size_t>((logLength > 1) ? logLength : 1));
        glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
        std::cout << (&fragShaderError[0]) << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        std::cout << "linking program" << std::endl;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> programError(static_cast<size_t>((logLength > 1) ? logLength : 1));
        glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
        std::cout << (&programError[0]) << std::endl;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}
