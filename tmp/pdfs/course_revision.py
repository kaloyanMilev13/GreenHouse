link=ns['link']
c1=link('Introduction to the Internet of Things and Embedded Systems','https://www.coursera.org/learn/iot')
c2=link('Introduction to Embedded Systems Software and Development Environments','https://www.coursera.org/learn/introduction-embedded-systems')
ns['intro']='Minimum outcome: complete at least TWO full Coursera courses, including required assessments. Budget: 54 hours over 12 weeks, averaging 4.5 hours/week. Course 1: 14 hours; Course 2: 20 hours; supporting theory and greenhouse experiments: 14 hours; evidence and reviews: 6 hours. No additional hardware is assumed.'
ns['rows']=[
['1-2','Embedded-system components, sensors/actuators and design constraints. Start functions, units, voltage, current and resistance to explain physical inputs.',
'<b>Course 1:</b> '+c1+' (UC Irvine, Ian Harris). Complete modules 1-2. Chosen to turn my existing IoT implementation experience into system-level reasoning.',
'<b>W1:</b> check assessment access; complete module 1.<br/><b>W2:</b> complete module 2; draw the greenhouse sensor-to-software-to-output path. Reserve 7 h for coursework and 2 h for supporting work/evidence.',
'Module assessments submitted/passed as required; labelled architecture with one power, timing and reliability constraint. Explain sensor versus actuator and identify one assumption.'],
['3-4','Hardware/software trade-offs and network layers; distinguish Wi-Fi, IP, transport and application protocols. Apply linear calibration and Ohm\'s law.',
'<b>Course 1:</b> complete modules 3-4 and all remaining requirements. Support: '+link('OpenStax linear functions','https://openstax.org/books/college-algebra-2e/pages/4-1-linear-functions')+' and '+link('All About Circuits DC','https://www.allaboutcircuits.com/textbook/direct-current/')+' for brief worked calculations.',
'<b>W3:</b> module 3; calculate a two-point calibration and resistor current.<br/><b>W4:</b> module 4; map greenhouse messages onto network layers. 7 h coursework + 2 h supporting work/evidence.',
'<b>Course 1 complete by W4:</b> completion record and required assessment results. Explain one hardware/software choice and correctly solve a changed calibration input without copying.'],
['5-6','C readiness; build tools and version control. Understand how source code becomes an executable rather than treating the build as a black box.',
'<b>Course 2:</b> '+c2+' (Colorado Boulder, Alex Fosdick). Begin module 1 and the required environment setup. Chosen for assessed C and development-tool practice.',
'<b>W5:</b> check pointers, arrays and function basics; begin setup/module 1.<br/><b>W6:</b> finish module 1 tasks; reproduce a clean build. 7 h coursework + 2 h C revision/evidence.',
'Build log, versioned source and submitted module work. Explain array bounds and pointer lifetime. <b>Fit review with Iman:</b> one appealing task, one difficulty and one open question.'],
['7-8','Compilation, linking and Make; memory regions, storage lifetime and stack/heap distinctions. Connect program organisation to predictable embedded behaviour.',
'<b>Course 2:</b> module 2, then start module 3. Use '+link('GNU C manual','https://www.gnu.org/software/c-intro-and-ref/manual/c-intro-and-ref.html')+' only to repair specific pointer/storage gaps; coursework remains the priority.',
'<b>W7:</b> complete module 2 build work.<br/><b>W8:</b> begin module 3; annotate where example variables and code reside. 7 h coursework + 2 h supporting work/evidence.',
'Submitted assessments, reproducible build and memory sketch. Predict what changes when a local variable becomes static; explain compile errors versus link errors.'],
['9-10','Memory manipulation and validation. Apply the course concepts independently and finish the assessed project before the final review period.',
'<b>Course 2:</b> finish module 3 and module 4 final assessment, including required peer-review work. Retain the course toolchain for its assignments; transfer concepts to ESP32 separately.',
'<b>W9:</b> finish module 3 tasks.<br/><b>W10:</b> submit final project/reviews; check completion status. 6 h coursework + 3 h supporting work/evidence. Reserve later time for revisions or outstanding grades.',
'<b>Course 2 completion target: W10.</b> Final submission, build instructions and boundary-case results. Explain one bug and its cause; pending grading is recorded as pending, not completed.'],
['11-12','Consolidate reliable IoT and timing; introduce rate and accumulation as future control prerequisites. Secure the two-course completion target before extending scope.',
link('Espressif GPTimer','https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/timer.html')+' and '+link('MQTT specification','https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html')+' for one experiment; '+link('3Blue1Brown derivatives','https://www.3blue1brown.com/lessons/derivatives/')+' for a short maths bridge.',
'<b>W11:</b> resolve course revisions; choose ONE timing or MQTT outage experiment.<br/><b>W12:</b> explain results and calculate slope/accumulated error from synthetic data. 9 h total, including final documentation and coach review.',
'<b>Minimum: TWO completed courses</b> with completion records and passed required work. Show an interval-error table OR numbered-message outage log. If grading delays completion, agree a dated follow-up; do not count submission alone.']]
ns['weekly']=[('Theory and coursework - 2 h','Follow the scheduled course material; after both courses, use this time for supporting maths/electronics.'),('Practice and assessments - 2 h','Complete required assignments first, then the small greenhouse experiment. Course assignments count as practical work, not extra hours.'),('Evidence - 0.5 h','Save results, code and a short explanation of the principle, design choice and limitation. Total: 4.5 h/week.')]
ns['evalitems']=[
'<b>Complete at least two courses:</b> aim for Course 1 by week 4 and Course 2 by week 10, with weeks 11-12 for corrections. Show completion records or certificates and required passing results; watching videos alone does not count.',
'<b>Explain:</b> at each fortnightly checkpoint, give a five-minute explanation without tutorial notes, covering the principle, implementation, alternative and limitation.',
'<b>Transfer:</b> produce at least three independent results: a calibration/circuit calculation, a clean build with memory explanation, and a greenhouse timing or outage experiment. Predict one changed input for each.',
'<b>Reproduce and diagnose:</b> keep inputs, versions, code and results. Re-run one example with Iman and explain one failed case and correction.',
'<b>Review fit and workload:</b> at weeks 6 and 12, use the hours log, two concrete experiences and one remaining knowledge gap to decide whether to continue Embedded Systems or investigate another direction.']
