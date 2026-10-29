from reportlab.pdfgen import canvas
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from pathlib import Path
import re
base=Path('/home/kala420/Projects/fontys/Greenhouse/output/pdf')
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
pdfmetrics.registerFont(TTFont('PlanSans','/usr/share/fonts/noto/NotoSans-Regular.ttf'))
pdfmetrics.registerFont(TTFont('PlanSansBold','/usr/share/fonts/noto/NotoSans-Bold.ttf'))
pdfmetrics.registerFontFamily('PlanSans',normal='PlanSans',bold='PlanSansBold')
styles=getSampleStyleSheet()
styles.add(ParagraphStyle(name='BodyX',fontName='PlanSans',fontSize=9.5,leading=12.5,spaceAfter=9,textColor=colors.HexColor('#253346')))
styles.add(ParagraphStyle(name='CellX',parent=styles['BodyX'],fontSize=8.6,leading=10.7,spaceAfter=0))
styles.add(ParagraphStyle(name='SmallX',parent=styles['BodyX'],fontSize=8.5,leading=11))
styles.add(ParagraphStyle(name='HeadingX',fontName='PlanSansBold',fontSize=15,leading=19,spaceBefore=10,spaceAfter=10,textColor=colors.HexColor('#145e65')))
styles.add(ParagraphStyle(name='TitleX',parent=styles['HeadingX'],fontSize=23,leading=27))
styles.add(ParagraphStyle(name='TH',fontName='PlanSansBold',fontSize=8.5,leading=11,textColor=colors.white))
def link(label,url): return f'<a href="{url}" color="#146e80"><u>{label}</u></a>'
def p(t,s='BodyX'): return Paragraph(t,styles[s])
rationale='I am considering Embedded Systems (Intelligent Technologies) because I enjoy C and understanding how software interacts with hardware. I have started using C, ESP32, Wi-Fi and MQTT in an IoT greenhouse project, and explored alternative communication protocols. This gives me a concrete system in which to investigate sensors, timing and communication decisions. My interest in registers, peripherals, interrupts and digital logic is a direction to investigate, not evidence of mastery. Embedded engineering requires reasoning about physical behaviour, memory, timing and failure as well as writing working code. After discussing this gap with Iman, I want to build the mathematics, electronics and computer science needed to explain why a design works and when another approach is better. This makes the specialisation a considered next step, which I will test through the evidence below.'
intro='Scope: 12 weeks at 4.5 hours/week (54 hours). Selected lessons and small experiments, not complete courses. I assume access to my existing ESP32 development setup; no additional sensors, actuators or measuring instruments are assumed. Synthetic data and calculations are acceptable if clearly labelled.'
rows=[
['1-2<br/><b>Models &amp; circuits</b>',
'Functions, slope, units; voltage, current, resistance, power, series circuits, voltage dividers and capacitance. <b>Why:</b> turn sensor values and circuit choices into quantities I can justify.',
link('OpenStax: College Algebra 2e, 3.1 &amp; 4.1','https://openstax.org/books/college-algebra-2e/pages/4-1-linear-functions')+' for function examples; '+link('All About Circuits: DC','https://www.allaboutcircuits.com/textbook/direct-current/')+', selected voltage/current, Ohm\'s law, divider and capacitor lessons for physical meaning. <b>Entry:</b> arithmetic and rearranging simple equations.',
'<b>W1:</b> diagnose algebra gaps; derive y = ax + b from two synthetic calibration points.<br/><b>W2:</b> calculate divider output, current and power; sketch capacitor charging and explain stored charge.',
'One calculation sheet and labelled circuit. Check units and verify three calibration inputs, including one outside the fitted range. Explain divider loading and why an ideal calculation alone does not establish GPIO compatibility.'],
['3-4<br/><b>C &amp; memory</b>',
'Binary/hex, unsigned bit masks, pointers, arrays, bounds, storage lifetime; CPU, RAM, flash and memory-mapped I/O. <b>Why:</b> understand how data and peripheral state become machine operations.',
link('GNU C Language Manual','https://www.gnu.org/software/c-intro-and-ref/manual/c-intro-and-ref.html')+': Integer Types, Bitwise Operations, Pointers, Arrays and Storage Class Specifiers. Direct explanations with small code examples. <b>Entry:</b> basic C variables, loops and functions; revise these first if needed.',
'<b>W3:</b> draw memory for a fixed array and pointer; trace a bit-mask update.<br/><b>W4:</b> write a C function converting an array of synthetic sensor readings, passing its length explicitly. Avoid dynamic allocation.',
'Compiling program, memory sketch and normal/empty/boundary cases. Explain array bounds, pointer lifetime and unsigned wraparound; correctly predict a changed input before running it.'],
['5-6<br/><b>Peripherals &amp; timing</b>',
'GPIO levels, Boolean masks, registers, timers, polling versus interrupts, sampling interval and jitter. Identify UART, I2C and SPI roles; implement only timing. <b>Why:</b> a correct reading also needs predictable timing.',
link('Espressif: GPIO &amp; RTC GPIO','https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html')+' and '+link('GPTimer','https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/timer.html')+', selected configuration and callback examples. <b>Entry:</b> phase 2 and an existing build/flash workflow. Match docs to installed framework and chip; no framework migration.',
'<b>W5:</b> trace one GPIO setting from API to peripheral; draw timer-to-handler flow.<br/><b>W6:</b> timestamp 100 periodic events with network activity off/on. Use the timer API available in my setup; defer printing/network work out of the ISR.',
'Annotated flow diagram, code and min/mean/max interval error. Explain polling, ISR work and observed jitter; distinguish measurements from a worst-case timing guarantee. <b>Fit review:</b> record one appealing task, one difficulty and one open question.'],
['7-8<br/><b>Reliable IoT</b>',
'Wi-Fi link, IP addressing, TCP versus UDP; MQTT topics, QoS 0/1, retained state and reconnects; HTTP/CoAP trade-offs. <b>Why:</b> justify MQTT and distinguish delivery from freshness or successful actuation.',
link('OASIS MQTT 3.1.1','https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html')+' sections 3.3 and 4.3; '+link('HTTP Semantics','https://httpwg.org/specs/rfc9110.html')+' sections 3 and 9.3; '+link('CoAP RFC 7252','https://www.rfc-editor.org/rfc/rfc7252')+' sections 2 and 4. Short primary-source excerpts for a defensible comparison. <b>Entry:</b> existing MQTT experiment and timing logs.',
'<b>W7:</b> draw MQTT/TCP/IP/Wi-Fi layers; compare publish/subscribe with request/response.<br/><b>W8:</b> send numbered, timestamped readings, interrupt Wi-Fi and reconnect. Log loss, duplicates and stale data with the supported QoS setting.',
'Comparison table: pattern, transport, overhead assumptions, delivery and security; outage log with observed versus expected behaviour. Explain QoS 1 duplicates, retained-message age, bounded buffering and TLS/authentication. Name one scenario favouring HTTP and one favouring CoAP.'],
['9-10<br/><b>Maths for feedback</b>',
'Derivatives as rate, integrals as accumulation; finite differences, sums and a first-order differential equation. <b>Why:</b> P, I and D need meanings and units before tuning makes sense.',
'3Blue1Brown: '+link('The paradox of the derivative','https://www.3blue1brown.com/lessons/derivatives/')+' and '+link('Integration and the fundamental theorem of calculus','https://www.3blue1brown.com/lessons/integration/')+'. Visual intuition, followed by my own calculations. <b>Entry:</b> phase 1 functions/units and phase 3 sampling; replay algebra work if these are weak.',
'<b>W9:</b> calculate temperature slopes and accumulated error for a short synthetic series.<br/><b>W10:</b> step through dT/dt = (Tenv - T)/tau + k*u using Euler updates. State units; compare results at dt and dt/2.',
'Worked values and two response curves, with parameters labelled hypothetical. Explain tau, slope, area and discretisation error. <b>Gate:</b> compute difference/dt and sum(error*dt) unaided before starting PI control.'],
['11-12<br/><b>Feedback trial</b>',
'Feedback, setpoint/error, P and PI, saturation, windup, noise, sampling and signs of instability; interpret D only. <b>Why:</b> assess when feedback improves on simple on/off control.',
link('MathWorks: Understanding PID Control','https://www.mathworks.com/videos/series/understanding-pid-control.html')+', parts 1-3: PID basics, anti-windup and derivative noise. Conceptual videos; use my C model without MATLAB. <b>Entry:</b> phase 5 gate; full PID tuning and stability proofs are later work.',
'<b>W11:</b> reuse the thermal model; compare on/off with hysteresis against P.<br/><b>W12:</b> add PI with output limits and conditional integration. Repeat with a disturbance and a larger sample interval. Predict each change first.',
'Overlaid curves and a table of peak overshoot, final error and settling time (state tolerance/horizon; report if not settled). Explain saturation, why D reacts to noise and why simulated stability is not proof for hardware. Final coach review.']]
weekly=[('Theory - 2 h','Read/watch only the named extracts; solve or sketch before opening a worked answer.'),('Practice - 1.75 h','Run one controlled experiment: prediction, single change, recorded result.'),('Evidence - 0.75 h','Keep code, parameters, diagram/plot and a 150-word explanation: why, result, limitation, next question.')]
longterm=[('<b>Mathematics and electronics:</b> extend algebra into trigonometry (phase and periodic signals), vectors/matrices (coordinates and coupled systems), then derivatives, integrals and differential equations in depth. Use these to analyse RC dynamics and sensor/actuator response; study logic gates and interface electrical limits.'),('<b>Embedded and real-time systems:</b> deepen architecture, registers, ADC/PWM and UART/I2C/SPI through one peripheral at a time. Then study RTOS scheduling, deadlines, task/ISR communication and race conditions. Demonstrate a measured timing budget and recovery from a failed peripheral.'),('<b>Control engineering, then robotics:</b> progress to modelling, transfer functions, stability analysis, sampling/aliasing, filtering and full PID with actuator constraints. After Linux CLI and C++ or Python basics, explore ROS2 nodes, topics, services and QoS using the '+link('official ROS2 tutorials (source)','https://github.com/ros2/ros2_documentation/blob/jazzy/source/Tutorials.rst')+'. ROS2 supports communication between robotics components; it does not replace low-level control or guarantee deadlines. Start in simulation; choose an OS-compatible supported release at that time.')]
evalitems=[('<b>Explain:</b> at each fortnightly checkpoint, give a five-minute explanation without tutorial notes: principle, implementation, alternative and limitation. Correct gaps before moving on.'),('<b>Predict and transfer:</b> make at least one written prediction per phase and solve one changed-input task without copying a solution. Explain any mismatch using units, code or a model.'),('<b>Reproduce:</b> keep six evidence bundles with source links, code/version, inputs, assumptions and results. Re-run one experiment with Iman from its recorded steps.'),('<b>Reason about failure:</b> by week 12, show evidence for an array-boundary case, a network outage and actuator saturation in simulation; distinguish observations from guarantees.'),('<b>Decide on fit:</b> review at weeks 6 and 12. Use two concrete examples of enjoying or disliking hardware/software reasoning, one persistent gap and the workload log to decide: continue, extend foundations or investigate another specialisation.')]
flow=[]
def add(t,s='BodyX'): flow.append(p(t,s))
def table(rs):
    heads=['Period','Learning objectives','Resource and reason for choosing it','Practical application','Deliverable / completion criteria']
    data=[[p(h,'TH') for h in heads]]+[[p(c.split('<br/>')[0] if j==0 else c,'CellX') for j,c in enumerate(r)] for r in rs]
    t=Table(data,colWidths=[45,110,124,112,120],repeatRows=1,hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),colors.HexColor('#145e65')),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.HexColor('#edf4f5'),colors.HexColor('#f7f9fb')]),('LINEBELOW',(0,0),(-1,-1),0.5,colors.HexColor('#cad8dc'))]))
    flow.append(t)
add('Embedded Systems study plan','TitleX')
add('FONTYS ICT  /  FIRST YEAR  /  COACH: IMAN','SmallX')
add('A. Rationale - Why Embedded Systems?','HeadingX');add(rationale)
add('B. Learning roadmap - 12 weeks','HeadingX');add(intro,'SmallX');table(rows[:3])
flow.append(PageBreak())
add('B. Learning roadmap - continued','HeadingX');add('Order: quantities and circuits > C and memory > timing > network reliability > calculus > feedback. Each phase receives 9 hours. Later work builds on the relevant completion gates.','SmallX');table(rows[3:])
flow.append(PageBreak())
add('C. Weekly routine','HeadingX')
for name,txt in weekly:add('<b>'+name+'</b> - '+txt)
add('Total: 4.5 h/week. A fifth hour is optional catch-up, not new content. If a gate fails, use the next practice session to repair the gap and reduce later scope. If necessary, finish week 12 with P control and defer PI; do not compress the foundations.')
add('D. Longer-term direction','HeadingX')
for t in longterm:add(t)
add('E. Progress evaluation','HeadingX')
for i,t in enumerate(evalitems,1):add(str(i)+'. '+t)
add('Resource access and prerequisites','HeadingX')
add('Resource titles, linked content and relevance checked on 29 October 2026. The core readings/video pages are publicly accessible; no paid course enrolment or certificate is required. Study only the named sections. Entry requirements in the table are my study readiness checks, not provider-issued admissions requirements. GNU material includes extensions: focus on portable C and verify target-specific details. Espressif API versions vary; select the installed framework/chip version. The ROS2 rendered documentation blocked automated access, so its official source was verified instead.','SmallX')
def footer(c,doc):
    c.setStrokeColor(colors.HexColor('#cad8dc'));c.line(42,35,553,35)
    c.setFont('PlanSans',8);c.setFillColor(colors.HexColor('#657582'));c.drawString(42,23,'Embedded Systems | 12-week foundation plan | 29 October 2026');c.drawRightString(553,23,str(doc.page))
doc=SimpleDocTemplate(str(base/'embedded-systems-study-plan.pdf'),pagesize=(595.28,841.89),rightMargin=42,leftMargin=42,topMargin=32,bottomMargin=46,title='Embedded Systems Self-study Plan',author='')
doc.build(flow,onFirstPage=footer,onLaterPages=footer)
def md(t):
    t=re.sub(r'<a href="([^"]+)"[^>]*><u>(.*?)</u></a>',r'[\2](\1)',t)
    return t.replace('<b>','**').replace('</b>','**').replace('<br/>','<br>').replace('&amp;','&')
text='# Embedded Systems self-study plan\n\nFontys ICT | First year | Coach: Iman\n\n## A. Rationale - Why Embedded Systems?\n\n'+rationale+'\n\n## B. Learning roadmap - 12 weeks\n\n'+intro+'\n\n| Period | Learning objectives | Resource and reason for choosing it | Practical application | Deliverable / completion criteria |\n|---|---|---|---|---|\n'
for r in rows:text+='| '+' | '.join(md(c) for c in r)+' |\n'
text+='\n## C. Weekly routine\n\n'+ '\n\n'.join('**'+a+'** - '+b for a,b in weekly)+'\n\nTotal: 4.5 hours/week. An optional fifth hour is for catch-up. If a gate fails, repair the gap in the next practice session and reduce later scope; defer PI if necessary.\n\n## D. Longer-term direction\n\n'+'\n\n'.join(md(t) for t in longterm)+'\n\n## E. Progress evaluation\n\n'+'\n\n'.join(str(i)+'. '+md(t) for i,t in enumerate(evalitems,1))+'\n\nResource pages checked 29 October 2026. Public readings/video pages; no paid enrolment required. Readiness checks are planning judgements, not formal admissions requirements. Match Espressif documentation to the installed chip/framework; focus on portable C in GNU documentation. ROS2 source verified because rendered docs blocked automated access.\n'
(base/'embedded-systems-study-plan.md').write_text(text)
print(base/'embedded-systems-study-plan.pdf')
