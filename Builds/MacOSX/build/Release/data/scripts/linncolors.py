import linnstrument

linn = linnstrument.get("linnstrumentcontrol")

def on_pulse():
   global linn
   step = bespoke.get_step(16)
   for x in range(25):
      if x == step % 25:
         for y in range(8):
            linn.set_color(x,y,[linn.Orange,linn.Green][(step//25)%2])

 