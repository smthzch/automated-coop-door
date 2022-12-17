// main.c

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdlib.h>

#define ITER 15
#define NREAD 48
#define NSKIP 6

long TIMELAP = 120; //5sec * 12*10
volatile long time = 0;
volatile int sample = 0;//6; //add every 6th sample to readings

volatile int readings[NREAD];
volatile int classes[NREAD];
volatile double mu0=100, mu1=30;
volatile int on=0;

//
int rand_(){
  int u = rand();
  if(u>16383) return 1;
  else return 0;
}

//
void addReading(int reading)
{
  int i = 0;
  for(i=0; i<(NREAD-1); i++)
  {
    readings[i] = readings[i+1];
  }
  readings[(NREAD-1)] = reading; 
}

//
int getADC()
{
  ADMUX = 0b00100011;
  
  //ADCSRA = (1<<ADPS0)|(1<<ADPS1); //start conversion select pin input
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADSC);
  
  while(!(ADCSRA&(1<<ADIF))){} //wait for conversion to complete

  int raw = ADCH; 
  return raw;
}

//options for classification include:
//hmm
//kmeans
//gaussian mm w/ EM

double npdf(double x, double mu, double sd)
{
  return (pow(x-mu,2)/(-2*sd*sd))-log(sd);
}

int classify(int x)
{
  double x_ = (double)x;
  double l0 = pow(x_-mu0,2);//npdf((double)x, mu0, sd0);
  double l1 = pow(x_-mu1,2);//npdf((double)x, mu1, sd1);
  if(l0<l1) return 0;
  else return 1;
}

void fit()
{
  int n = 0;
  int i = 0;
  for(n=0; n<NREAD; n++)
  {
      classes[n] = rand_();
  }
  for(i=0; i<ITER; i++)
  {
    //m-step
    mu0=0;
    mu1=0;
    double n0=0, n1=0;
    for(n=0; n<NREAD; n++)
    {
      if(classes[n]==0)
      {
        n0++;
        mu0 += readings[n];
        
      }else
      {
        n1++;
        mu1 += readings[n];
      }      
    }
    mu0 /= n0;
    mu1 /= n1;
    //e-step
    for(n=0; n<NREAD; n++)
    {
      classes[n] = classify(readings[n]);
    }
  }
  //make sure ordering correct
  if(mu0<mu1)
  {
    double temp = mu0;
    mu0 = mu1;
    mu1 = temp;
  }
}

//set timer interrupt
ISR(TIMER1_OVF_vect)
{
  TIMSK &= ~(1<<TOIE1); //disable timer
  time++;
  
  if(time>TIMELAP)
  {
    int new_reading = getADC();
    fit();
    int DoN = classify(new_reading);
    
    if(DoN==0)
    {
      //open gate
      PORTB &= ~(1<<PB1);
      PORTB |= (1<<PB0);
    }else
    {
      //close gate
      PORTB |= (1<<PB1);
      PORTB &= ~(1<<PB0);
    }
    //external led counter
    if(on==1){
      on=0;
      
    }else{
      on=1;
      
    }
    //add every nskip+1 sample to records
    if((sample%NSKIP)==0)
    {
      addReading(new_reading);
      sample = 0;
    }else sample++;
    time = 0;
  }
  TCNT1 = 0;
  TIMSK |= (1<<TOIE1);
}

//
int main (void)
{
  //setup
  //adc
  ADMUX = 0b00100011; //Vcc ref, right adjusted, pin5
  
  //gate motor controlpins
  //PORTB = (1<<PB0); //set gate to open on startup
  DDRB = 0b00000011; // set PB0&1 to be output
  
  //dummy data
  int i = 0;
  for(i=0; i<NREAD; i++)
  {
    if(i<(NREAD/2)) 
    {
      readings[i]=mu1;
      classes[i]=1;
    }
    else 
    {
      readings[i]=mu0;
      classes[i]=0;
    }
  }
  
  TCCR1 = (15<<CS10);//15=5sec,11=1sec
  TIMSK = (1<<TOIE1);
  sei();
  while (1) {
    //wait for the timer interrupts
  }
 
  return 1;
}
