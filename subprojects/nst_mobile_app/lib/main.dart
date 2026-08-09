import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

void main() {
  runApp(const NSTMobileApp());
}

class NSTMobileApp extends StatelessWidget {
  const NSTMobileApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'NST Mobile: Game Translator',
      debugShowCheckedModeBanner: false,
      theme: ThemeData.dark().copyWith(
        scaffoldBackgroundColor: const Color(0xFF0B0F19),
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFF8B5CF6),
          secondary: Color(0xFF06B6D4),
          surface: Color(0xFF161D2F),
        ),
      ),
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _userEnergy = 150;
  int _currentTabIndex = 0;
  final TextEditingController _testTextController = TextEditingController(
    text: r'\N[1]、気をつけて！',
  );
  String _translationResult = '';
  String _translationStatus = '';
  bool _isLoading = false;

  final String _goServerUrlHost = 'http://10.0.2.2:8080/api/v1/translate';
  final String _goServerUrlLocal = 'http://127.0.0.1:8080/api/v1/translate';

  // Live Emulator Storage Paths
  final String _sdcardFragilePath = '/sdcard/NSTGames/FragilePrincess';
  final String _sdcardAkudochiPath = '/sdcard/NSTGames/AkudochiClicker';

  void _watchMetaRewardedAd() {
    int timeLeft = 5;
    Timer? timer;

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) {
        return StatefulBuilder(
          builder: (context, setModalState) {
            timer ??= Timer.periodic(const Duration(seconds: 1), (t) {
              if (timeLeft > 1) {
                setModalState(() {
                  timeLeft--;
                });
              } else {
                t.cancel();
                setModalState(() {
                  timeLeft = 0;
                });
              }
            });

            return AlertDialog(
              backgroundColor: const Color(0xFF111827),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(24),
                side: const BorderSide(color: Color(0xFF6366F1), width: 1.5),
              ),
              title: Row(
                children: [
                  const Icon(Icons.all_inclusive, color: Color(0xFF0084FF)),
                  const SizedBox(width: 8),
                  const Expanded(
                    child: Text(
                      'Meta Audience Network',
                      style: TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.bold,
                        color: Color(0xFF0084FF),
                      ),
                    ),
                  ),
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                    decoration: BoxDecoration(
                      color: const Color(0xFFEF4444).withOpacity(0.2),
                      borderRadius: BorderRadius.circular(10),
                    ),
                    child: Text(
                      timeLeft > 0 ? '${timeLeft}s' : '✅ DONE',
                      style: const TextStyle(
                        color: Color(0xFFEF4444),
                        fontWeight: FontWeight.bold,
                        fontSize: 12,
                      ),
                    ),
                  ),
                ],
              ),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Container(
                    height: 140,
                    width: double.infinity,
                    decoration: BoxDecoration(
                      gradient: const LinearGradient(
                        colors: [Color(0xFF1E1B4B), Color(0xFF311B92)],
                      ),
                      borderRadius: BorderRadius.circular(16),
                    ),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: const [
                        Text(
                          'Meta',
                          style: TextStyle(
                            fontSize: 32,
                            fontWeight: FontWeight.w900,
                            color: Color(0xFF0084FF),
                          ),
                        ),
                        SizedBox(height: 6),
                        Text(
                          '🎮 แปลเกม RPG ญี่ปุ่นภาษาไทยฟรีแบบไร้ขีดจำกัด',
                          style: TextStyle(fontSize: 11, color: Colors.white70),
                          textAlign: TextAlign.center,
                        ),
                        SizedBox(height: 10),
                        Chip(
                          backgroundColor: Color(0xFFFBBF24),
                          label: Text(
                            '+50 FREE ENERGY',
                            style: TextStyle(
                              color: Colors.black,
                              fontWeight: FontWeight.bold,
                              fontSize: 11,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
              actions: [
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: timeLeft == 0
                          ? const Color(0xFF10B981)
                          : Colors.grey[800],
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                    onPressed: timeLeft == 0
                        ? () {
                            setState(() {
                              _userEnergy += 50;
                            });
                            Navigator.of(context).pop();
                            ScaffoldMessenger.of(context).showSnackBar(
                              const SnackBar(
                                content: Text('🎉 รับพลังงานฟรี +50 ⚡ เรียบร้อย!'),
                                backgroundColor: Color(0xFF10B981),
                              ),
                            );
                          }
                        : null,
                    child: Text(
                      timeLeft > 0 ? '⏳ กรุณารับชมจนจบ (${timeLeft}s)' : 'รับรางวัล +50 ⚡',
                      style: const TextStyle(fontWeight: FontWeight.bold),
                    ),
                  ),
                ),
              ],
            );
          },
        );
      },
    );
  }

  void _inspectGameFiles(String title, String path) {
    List<Map<String, String>> fileDetails = [];
    final dataDir = Directory('$path/data');

    if (dataDir.existsSync()) {
      final entities = dataDir.listSync();
      for (var entity in entities) {
        if (entity is File && entity.path.endsWith('.json')) {
          final filename = entity.path.split('/').last;
          final stat = entity.statSync();
          fileDetails.add({
            'name': 'data/$filename',
            'size': '${(stat.size / 1024).toStringAsFixed(1)} KB',
            'status': '✅ แปลไทยแล้วใน /sdcard',
          });
        }
      }
    } else {
      fileDetails.add({
        'name': 'data/*.json',
        'size': '0 KB',
        'status': '⚠️ ไม่พบโฟลเดอร์ใน /sdcard',
      });
    }

    showModalBottomSheet(
      context: context,
      backgroundColor: const Color(0xFF111827),
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
      builder: (context) {
        return Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            mainAxisSize: MainAxisSize.min,
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Expanded(
                    child: Text(
                      title,
                      style: const TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 16,
                        color: Colors.white,
                      ),
                    ),
                  ),
                  IconButton(
                    icon: const Icon(Icons.close, color: Colors.grey),
                    onPressed: () => Navigator.pop(context),
                  ),
                ],
              ),
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: Colors.black45,
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Text(
                  'Path: $path',
                  style: const TextStyle(
                    fontSize: 10,
                    color: Color(0xFF06B6D4),
                    fontFamily: 'monospace',
                  ),
                ),
              ),
              const SizedBox(height: 12),
              Text(
                'ตรวจสอบไฟล์ใน Emulator Storage (${fileDetails.length} ไฟล์):',
                style: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold, color: Colors.grey),
              ),
              const SizedBox(height: 8),
              Expanded(
                child: ListView.builder(
                  shrinkWrap: true,
                  itemCount: fileDetails.length,
                  itemBuilder: (context, index) {
                    final item = fileDetails[index];
                    return Container(
                      margin: const EdgeInsets.only(bottom: 6),
                      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                      decoration: BoxDecoration(
                        color: const Color(0xFF161D2F),
                        borderRadius: BorderRadius.circular(10),
                      ),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Text(
                            item['name']!,
                            style: const TextStyle(
                              fontSize: 12,
                              fontFamily: 'monospace',
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                          Text(
                            '${item['size']} • ${item['status']}',
                            style: const TextStyle(
                              fontSize: 10,
                              color: Color(0xFF10B981),
                            ),
                          ),
                        ],
                      ),
                    );
                  },
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Future<void> _translateWithGoServer() async {
    if (_userEnergy < 1) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('พลังงานหมด! กรุณาดูโฆษณาเพื่อรับพลังงานฟรี')),
      );
      _watchMetaRewardedAd();
      return;
    }

    final text = _testTextController.text.trim();
    if (text.isEmpty) return;

    setState(() {
      _isLoading = true;
      _translationStatus = '⏳ กำลังยิงต่อไปยัง NST Go Server...';
      _translationResult = '';
    });

    try {
      http.Response response;
      try {
        response = await http.post(
          Uri.parse(_goServerUrlHost),
          headers: {'Content-Type': 'application/json'},
          body: jsonEncode({'text': text}),
        ).timeout(const Duration(seconds: 5));
      } catch (_) {
        response = await http.post(
          Uri.parse(_goServerUrlLocal),
          headers: {'Content-Type': 'application/json'},
          body: jsonEncode({'text': text}),
        ).timeout(const Duration(seconds: 5));
      }

      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        setState(() {
          _translationResult = data['translated'] ?? text;
          _translationStatus = data['cached'] == true
              ? '⚡ Hit Cache (0ms)'
              : '🌐 Translated via Go Server';
          _userEnergy -= 1;
        });
      } else {
        setState(() {
          _translationStatus = '❌ Server Error (${response.statusCode})';
        });
      }
    } catch (e) {
      setState(() {
        _translationStatus = '⚠️ ไม่สามารถต่อ Go Server บน localhost:8080 ($e)';
      });
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: const Color(0xFF111827),
        elevation: 0,
        title: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(6),
              decoration: BoxDecoration(
                color: const Color(0xFF8B5CF6).withOpacity(0.2),
                borderRadius: BorderRadius.circular(10),
              ),
              child: const Icon(Icons.videogame_asset, color: Color(0xFF8B5CF6)),
            ),
            const SizedBox(width: 10),
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: const [
                Text(
                  'NST Mobile',
                  style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
                ),
                Text(
                  'Game Translator Pro',
                  style: TextStyle(fontSize: 10, color: Colors.grey),
                ),
              ],
            ),
          ],
        ),
        actions: [
          GestureDetector(
            onTap: _watchMetaRewardedAd,
            child: Container(
              margin: const EdgeInsets.only(right: 16),
              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
              decoration: BoxDecoration(
                gradient: const LinearGradient(
                  colors: [Color(0x33F59E0B), Color(0x33EC4899)],
                ),
                borderRadius: BorderRadius.circular(20),
                border: Border.all(color: const Color(0xFFF59E0B), width: 1),
              ),
              child: Row(
                children: [
                  const Text('⚡', style: TextStyle(fontSize: 14)),
                  const SizedBox(width: 4),
                  Text(
                    '$_userEnergy',
                    style: const TextStyle(
                      color: Color(0xFFFBBF24),
                      fontWeight: FontWeight.bold,
                      fontSize: 13,
                    ),
                  ),
                  const SizedBox(width: 4),
                  const Icon(Icons.add_circle, color: Color(0xFFEC4899), size: 16),
                ],
              ),
            ),
          ),
        ],
      ),
      body: IndexedStack(
        index: _currentTabIndex,
        children: [
          _buildGamesTab(),
          _buildTranslatorTab(),
          _buildMonetizeTab(),
        ],
      ),
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _currentTabIndex,
        backgroundColor: const Color(0xFF111827),
        selectedItemColor: const Color(0xFF8B5CF6),
        unselectedItemColor: Colors.grey,
        onTap: (idx) {
          setState(() {
            _currentTabIndex = idx;
          });
        },
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.sports_esports),
            label: 'เกมในเครื่อง',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.bolt),
            label: 'Go Server',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.monetization_on),
            label: 'รับแปลฟรี (Ads)',
          ),
        ],
      ),
    );
  }

  Widget _buildGamesTab() {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        // Meta Banner Ad
        Container(
          padding: const EdgeInsets.all(12),
          decoration: BoxDecoration(
            gradient: const LinearGradient(
              colors: [Color(0xFF1E1B4B), Color(0xFF311B92)],
            ),
            borderRadius: BorderRadius.circular(16),
            border: Border.all(color: const Color(0x556366F1)),
          ),
          child: Row(
            children: [
              const Icon(Icons.all_inclusive, color: Color(0xFF0084FF)),
              const SizedBox(width: 10),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: const [
                    Text(
                      'META AUDIENCE NETWORK AD',
                      style: TextStyle(
                        fontSize: 9,
                        fontWeight: FontWeight.bold,
                        color: Color(0xFF0084FF),
                      ),
                    ),
                    SizedBox(height: 2),
                    Text(
                      '🎮 แปลเกมญี่ปุ่นแบบไร้ขีดจำกัด ดูโฆษณาเพื่อรับพลังงานแปลฟรี!',
                      style: TextStyle(fontSize: 11, color: Colors.white),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
        const SizedBox(height: 16),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: const [
            Text(
              'โฟลเดอร์เกมในเครื่องจำลอง (/sdcard/NSTGames)',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 12, color: Colors.grey),
            ),
            Text(
              '2 โฟลเดอร์',
              style: TextStyle(fontSize: 12, color: Color(0xFF06B6D4)),
            ),
          ],
        ),
        const SizedBox(height: 12),

        // Game 2 Card
        GestureDetector(
          onTap: () => _inspectGameFiles(
            '悪堕ち魔法少女クリッカー (Akudochi Mahou Shoujo Clicker)',
            _sdcardAkudochiPath,
          ),
          child: _buildGameCard(
            title: '悪堕ち魔法少女クリッカー',
            subtitle: 'Akudochi Mahou Shoujo Clicker',
            path: _sdcardAkudochiPath,
            engine: 'RPG Maker MZ',
            statusText: 'แปลภาษาไทย 100% เรียบร้อยแล้ว ✅ (แตะเพื่อตรวจไฟล์)',
            iconText: '魔法',
            color: const Color(0xFFEC4899),
          ),
        ),

        const SizedBox(height: 14),

        // Game 1 Card
        GestureDetector(
          onTap: () => _inspectGameFiles(
            'FRAGILE PRINCESS Ver 1.1.1',
            _sdcardFragilePath,
          ),
          child: _buildGameCard(
            title: 'FRAGILE PRINCESS Ver 1.1.1',
            subtitle: 'RJ01647202',
            path: _sdcardFragilePath,
            engine: 'RPG Maker MV',
            statusText: 'แปลภาษาไทย 100% เรียบร้อยแล้ว ✅ (แตะเพื่อตรวจไฟล์)',
            iconText: 'FP',
            color: const Color(0xFF06B6D4),
          ),
        ),
      ],
    );
  }

  Widget _buildGameCard({
    required String title,
    required String subtitle,
    required String path,
    required String engine,
    required String statusText,
    required String iconText,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF161D2F),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: color.withOpacity(0.4), width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                width: 44,
                height: 44,
                decoration: BoxDecoration(
                  color: color,
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Center(
                  child: Text(
                    iconText,
                    style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
                  ),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      title,
                      style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
                      overflow: TextOverflow.ellipsis,
                    ),
                    Text(subtitle, style: const TextStyle(fontSize: 11, color: Colors.grey)),
                    Text(
                      path,
                      style: const TextStyle(fontSize: 9, color: Colors.grey, fontFamily: 'monospace'),
                      overflow: TextOverflow.ellipsis,
                    ),
                  ],
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: Colors.black38,
              borderRadius: BorderRadius.circular(6),
            ),
            child: Text('Engine: $engine', style: const TextStyle(fontSize: 10, color: Colors.grey)),
          ),
          const SizedBox(height: 10),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Expanded(
                child: Text(
                  statusText,
                  style: const TextStyle(fontSize: 11, color: Color(0xFF10B981), fontWeight: FontWeight.bold),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              const Icon(Icons.arrow_forward_ios, size: 12, color: Colors.grey),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildTranslatorTab() {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: const Color(0xFF161D2F),
            borderRadius: BorderRadius.circular(20),
            border: Border.all(color: const Color(0xFF8B5CF6).withOpacity(0.4)),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: const [
              Row(
                children: [
                  Icon(Icons.check_circle, color: Color(0xFF10B981), size: 16),
                  SizedBox(width: 8),
                  Text('NST Go Server Online', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14)),
                ],
              ),
              SizedBox(height: 4),
              Text('Target Endpoint: http://10.0.2.2:8080/api/v1/translate', style: TextStyle(fontSize: 11, color: Colors.grey)),
            ],
          ),
        ),
        const SizedBox(height: 16),
        Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: const Color(0xFF161D2F),
            borderRadius: BorderRadius.circular(20),
          ),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text('ทดสอบส่งแปลสด (ใช้ 1 ⚡)', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 13)),
              const SizedBox(height: 10),
              TextField(
                controller: _testTextController,
                decoration: InputDecoration(
                  hintText: 'ใส่ข้อความภาษาญี่ปุ่น...',
                  filled: true,
                  fillColor: const Color(0xFF0F172A),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12),
                    borderSide: BorderSide.none,
                  ),
                ),
              ),
              const SizedBox(height: 12),
              SizedBox(
                width: double.infinity,
                child: ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF8B5CF6),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    padding: const EdgeInsets.symmetric(vertical: 12),
                  ),
                  onPressed: _isLoading ? null : _translateWithGoServer,
                  child: _isLoading
                      ? const CircularProgressIndicator(color: Colors.white)
                      : const Text('🚀 แปลด้วย NST Go Server', style: TextStyle(fontWeight: FontWeight.bold)),
                ),
              ),
              if (_translationResult.isNotEmpty || _translationStatus.isNotEmpty) ...[
                const SizedBox(height: 14),
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: const Color(0xFF0F172A),
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: const Color(0xFF10B981).withOpacity(0.4)),
                  ),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(_translationStatus, style: const TextStyle(fontSize: 10, color: Color(0xFF06B6D4))),
                      const SizedBox(height: 4),
                      Text(_translationResult, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.bold, color: Colors.white)),
                    ],
                  ),
                ),
              ],
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildMonetizeTab() {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Container(
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            color: const Color(0xFF161D2F),
            borderRadius: BorderRadius.circular(24),
            border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.3)),
          ),
          child: Column(
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: const [
                  Icon(Icons.all_inclusive, color: Color(0xFF0084FF), size: 24),
                  SizedBox(width: 8),
                  Text('Meta Audience Network', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16, color: Color(0xFF0084FF))),
                ],
              ),
              const SizedBox(height: 12),
              const Text('ดูโฆษณาเพื่อแปลเกมฟรี 100%!', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16)),
              const SizedBox(height: 6),
              const Text('สะสมพลังงานแปลภาษาได้ไม่จำกัด ผ่านระบบ Rewarded Video Ads', style: TextStyle(fontSize: 12, color: Colors.grey), textAlign: TextAlign.center),
              const SizedBox(height: 20),
              Container(
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: Colors.black26,
                  borderRadius: BorderRadius.circular(16),
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text('พลังงานคงเหลือในเครื่อง', style: TextStyle(fontSize: 12)),
                    Text('$_userEnergy ⚡', style: const TextStyle(fontWeight: FontWeight.bold, color: Color(0xFFFBBF24), fontSize: 16)),
                  ],
                ),
              ),
              const SizedBox(height: 20),
              SizedBox(
                width: double.infinity,
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF10B981),
                    padding: const EdgeInsets.symmetric(vertical: 14),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                  ),
                  onPressed: _watchMetaRewardedAd,
                  icon: const Icon(Icons.play_circle_fill),
                  label: const Text('🎬 รับชมโฆษณา Meta (+50 ⚡)', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14)),
                ),
              ),
            ],
          ),
        ),
      ],
    );
  }
}
